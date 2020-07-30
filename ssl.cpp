#include <iostream>
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

static const int MAX_HANDSHAKES_PER_LOOP_ITERATION = 5;

struct SSLSocket {
    int  fd;
    SSL* ssl;
    bool wantsRead;
    bool wantsWrite;
};

struct SSlData {
    char *       ssl_read_input, *ssl_read_output;
    unsigned int ssl_read_input_length;
    unsigned int ssl_read_input_offset;
    SSLSocket*   ssl_socket;

    int last_write_was_msg_more;
    int msg_more;

    long long last_iteration_nr;
    int       handshake_budget;

    BIO*        shared_rbio;
    BIO*        shared_wbio;
    BIO_METHOD* shared_biom;
};

SSlData* ssl_data;

int passphrase_cb(char* buf, int size, int rwflag, void* u)
{
    const char* passphrase        = (const char*)u;
    size_t      passphrase_length = strlen(passphrase);
    std::memcpy(buf, passphrase, passphrase_length);
    // put null at end? no?
    return (int)passphrase_length;
}

int BIO_create_cb(BIO* bio)
{
    BIO_set_init(bio, true);
    return 1;
}

long BIO_ctrl_cb(BIO* bio, int cmd, long num, void* user)
{
    switch (cmd) {
        case BIO_CTRL_FLUSH: return 1;
        default: return 0;
    }
}

// GLobal variables

int write(SSLSocket* socket, const char* buf, int size)
{
    int ret = send(socket->fd, buf, size, MSG_NOSIGNAL);
    if (ret < 0) {
        perror("Write error");
        ret = 0;
    }
    return ret;
}

int BIO_write_method(BIO* bio, const char* data, int length)
{
    SSlData* sslData = (SSlData*)BIO_get_data(bio);

    int written = write(sslData->ssl_socket, data, length);

    if (!written) {
        BIO_set_flags(bio, BIO_FLAGS_SHOULD_RETRY | BIO_FLAGS_WRITE);
        return -1;
    }

    return written;
}

int BIO_read_method(BIO* bio, char* dst, int length)
{
    SSlData* sslData = (SSlData*)BIO_get_data(bio);

    // printf("BIO_s_custom_read\n");

    if (!sslData->ssl_read_input_length) {
        BIO_set_flags(bio, BIO_FLAGS_SHOULD_RETRY | BIO_FLAGS_READ);
        return -1;
    }

    if ((unsigned int)length > sslData->ssl_read_input_length) {
        length = sslData->ssl_read_input_length;
    }

    memcpy(dst, sslData->ssl_read_input + sslData->ssl_read_input_offset, length);

    sslData->ssl_read_input_offset += length;
    sslData->ssl_read_input_length -= length;
    return length;
}

struct us_internal_ssl_socket_t*
ssl_on_open(struct us_internal_ssl_socket_t* s, int is_client, char* ip, int ip_length)
{
    struct us_internal_ssl_socket_context_t* context =
        (struct us_internal_ssl_socket_context_t*)us_socket_context(0, &s->s);

    struct us_loop_t*     loop          = us_socket_context_loop(0, &context->sc);
    struct loop_ssl_data* loop_ssl_data = (struct loop_ssl_data*)loop->data.ssl_data;

    s->ssl                  = SSL_new(context->ssl_context);
    s->ssl_write_wants_read = 0;
    SSL_set_bio(s->ssl, loop_ssl_data->shared_rbio, loop_ssl_data->shared_wbio);

    BIO_up_ref(ssl_data->shared_rbio);
    BIO_up_ref(ssl_data->shared_wbio);

    SSL_set_accept_state(s->ssl);

    return (struct us_internal_ssl_socket_t*)context->on_open(s, is_client, ip, ip_length);
}

struct us_internal_ssl_socket_t*
ssl_on_close(struct us_internal_ssl_socket_t* s, int code, void* reason)
{
    struct us_internal_ssl_socket_context_t* context =
        (struct us_internal_ssl_socket_context_t*)us_socket_context(0, &s->s);

    SSL_free(s->ssl);

    return context->on_close(s, code, reason);
}

struct us_internal_ssl_socket_t* ssl_on_end(struct us_internal_ssl_socket_t* s)
{
    struct us_internal_ssl_socket_context_t* context =
        (struct us_internal_ssl_socket_context_t*)us_socket_context(0, &s->s);

    // whatever state we are in, a TCP FIN is always an answered shutdown

    /* Todo: this should report CLEANLY SHUTDOWN as reason */
    return us_internal_ssl_socket_close(s, 0, NULL);
}

void ssl_on_data(SSLSocket* socket, char* data, int length)
{
    // note: this context can change when we adopt the socket!

    // note: if we put data here we should never really clear it (not in write either, it still
    // should be available for SSL_write to read from!)
    ssl_data->ssl_read_input        = data;
    ssl_data->ssl_read_input_length = length;
    ssl_data->ssl_read_input_offset = 0;
    ssl_data->ssl_socket            = &s->s;
    ssl_data->msg_more              = 0;

    if (us_internal_ssl_socket_is_shut_down(s)) {

        int ret;
        if ((ret = SSL_shutdown(s->ssl)) == 1) {
            // two phase shutdown is complete here
            // printf("Two step SSL shutdown complete\n");

            /* Todo: this should also report some kind of clean shutdown */
            return us_internal_ssl_socket_close(s, 0, NULL);
        }
        else if (ret < 0) {

            int err = SSL_get_error(s->ssl, ret);

            if (err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL) {
                // we need to clear the error queue in case these added to the thread local
                // queue
                ERR_clear_error();
            }
        }

        // no further processing of data when in shutdown state
        return s;
    }

    // bug checking: this loop needs a lot of attention and clean-ups and check-ups
    int read = 0;
restart:
    while (1) {
        int just_read = SSL_read(s->ssl,
                                 loop_ssl_data->ssl_read_output + LIBUS_RECV_BUFFER_PADDING + read,
                                 LIBUS_RECV_BUFFER_LENGTH - read);

        if (just_read <= 0) {
            int err = SSL_get_error(s->ssl, just_read);

            // as far as I know these are the only errors we want to handle
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {

                // clear per thread error queue if it may contain something
                if (err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL) { ERR_clear_error(); }

                // terminate connection here
                return us_internal_ssl_socket_close(s, 0, NULL);
            }
            else {
                // emit the data we have and exit

                // assume we emptied the input buffer fully or error here as well!
                if (loop_ssl_data->ssl_read_input_length) {
                    return us_internal_ssl_socket_close(s, 0, NULL);
                }

                // cannot emit zero length to app
                if (!read) { break; }

                s = context->on_data(
                    s, loop_ssl_data->ssl_read_output + LIBUS_RECV_BUFFER_PADDING, read);
                if (us_socket_is_closed(0, &s->s)) { return s; }

                break;
            }
        }

        read += just_read;

        // at this point we might be full and need to emit the data to application and start
        // over
        if (read == LIBUS_RECV_BUFFER_LENGTH) {

            // emit data and restart
            s = context->on_data(
                s, loop_ssl_data->ssl_read_output + LIBUS_RECV_BUFFER_PADDING, read);
            if (us_socket_is_closed(0, &s->s)) { return s; }

            read = 0;
            goto restart;
        }
    }

    // trigger writable if we failed last write with want read
    if (s->ssl_write_wants_read) {
        s->ssl_write_wants_read = 0;

        // make sure to update context before we call (context can change if the user adopts the
        // socket!)
        context = (struct us_internal_ssl_socket_context_t*)us_socket_context(0, &s->s);

        s = (struct us_internal_ssl_socket_t*)context->sc.on_writable(&s->s); // cast here!
        // if we are closed here, then exit
        if (us_socket_is_closed(0, &s->s)) { return s; }
    }

    // check this then?
    if (SSL_get_shutdown(s->ssl) & SSL_RECEIVED_SHUTDOWN) {
        // printf("SSL_RECEIVED_SHUTDOWN\n");

        // exit(-2);

        // not correct anyways!
        s = us_internal_ssl_socket_close(s, 0, NULL);

        // us_
    }

    return s;
}

/* Lazily inits loop ssl data first time */
void us_internal_init_loop_ssl_data(struct us_loop_t* loop)
{
    if (!loop->data.ssl_data) {
        struct loop_ssl_data* loop_ssl_data = malloc(sizeof(struct loop_ssl_data));

        loop_ssl_data->ssl_read_output =
            malloc(LIBUS_RECV_BUFFER_LENGTH + LIBUS_RECV_BUFFER_PADDING * 2);

        OPENSSL_init_ssl(0, NULL);

        loop_ssl_data->shared_biom = BIO_meth_new(BIO_TYPE_MEM, "µS BIO");
        BIO_meth_set_create(loop_ssl_data->shared_biom, BIO_s_custom_create);
        BIO_meth_set_write(loop_ssl_data->shared_biom, BIO_s_custom_write);
        BIO_meth_set_read(loop_ssl_data->shared_biom, BIO_s_custom_read);
        BIO_meth_set_ctrl(loop_ssl_data->shared_biom, BIO_s_custom_ctrl);

        loop_ssl_data->shared_rbio = BIO_new(loop_ssl_data->shared_biom);
        loop_ssl_data->shared_wbio = BIO_new(loop_ssl_data->shared_biom);
        BIO_set_data(loop_ssl_data->shared_rbio, loop_ssl_data);
        BIO_set_data(loop_ssl_data->shared_wbio, loop_ssl_data);

        // reset handshake budget (doesn't matter what loop nr we start on)
        loop_ssl_data->last_iteration_nr = 0;
        loop_ssl_data->handshake_budget  = MAX_HANDSHAKES_PER_LOOP_ITERATION;

        loop->data.ssl_data = loop_ssl_data;
    }
}

/* Called by loop free, clears any loop ssl data */
void us_internal_free_loop_ssl_data(struct us_loop_t* loop)
{
    struct loop_ssl_data* loop_ssl_data = (struct loop_ssl_data*)loop->data.ssl_data;

    if (loop_ssl_data) {
        free(loop_ssl_data->ssl_read_output);

        BIO_free(loop_ssl_data->shared_rbio);
        BIO_free(loop_ssl_data->shared_wbio);

        BIO_meth_free(loop_ssl_data->shared_biom);

        free(loop_ssl_data);
    }
}

// we ignore reading data for ssl sockets that are
// in init state, if our so called budget for doing
// so won't allow it. here we actually use
// the kernel buffering to our advantage
int ssl_ignore_data(struct us_internal_ssl_socket_t* s)
{

    // fast path just checks for init
    if (!SSL_in_init(s->ssl)) { return 0; }

    // this path is run for all ssl sockets that are in init and just got data event from
    // polling

    struct us_loop_t*     loop          = s->s.context->loop;
    struct loop_ssl_data* loop_ssl_data = loop->data.ssl_data;

    // reset handshake budget if new iteration
    if (loop_ssl_data->last_iteration_nr != us_loop_iteration_number(loop)) {
        loop_ssl_data->last_iteration_nr = us_loop_iteration_number(loop);
        loop_ssl_data->handshake_budget  = MAX_HANDSHAKES_PER_LOOP_ITERATION;
    }

    if (loop_ssl_data->handshake_budget) {
        loop_ssl_data->handshake_budget--;
        return 0;
    }

    // ignore this data event
    return 1;
}

/* Per-context functions */
void* us_internal_ssl_socket_context_get_native_handle(
    struct us_internal_ssl_socket_context_t* context)
{
    return context->ssl_context;
}

struct us_internal_ssl_socket_context_t*
us_internal_create_child_ssl_socket_context(struct us_internal_ssl_socket_context_t* context,
                                            int context_ext_size)
{
    /* Create a new non-SSL context */
    struct us_socket_context_options_t       options = {0};
    struct us_internal_ssl_socket_context_t* child_context =
        (struct us_internal_ssl_socket_context_t*)us_create_socket_context(
            0,
            context->sc.loop,
            sizeof(struct us_internal_ssl_socket_context_t) + context_ext_size,
            options);

    /* The only thing we share is SSL_CTX */
    child_context->ssl_context = context->ssl_context;
    child_context->is_parent   = 0;

    return child_context;
}

struct us_internal_ssl_socket_context_t* us_internal_create_ssl_socket_context(
    struct us_loop_t* loop, int context_ext_size, struct us_socket_context_options_t options)
{
    /* If we haven't initialized the loop data yet, do so now */
    us_internal_init_loop_ssl_data(loop);

    /* We begin by creating a non-SSL context, passing same options */
    struct us_internal_ssl_socket_context_t* context =
        (struct us_internal_ssl_socket_context_t*)us_create_socket_context(
            0, loop, sizeof(struct us_internal_ssl_socket_context_t) + context_ext_size, options);

    /* Now update our options parameter since above function made a deep copy, and we want to
     * use that copy below */
    options = context->sc.options;

    /* Then we extend its SSL parts */
    context->ssl_context = SSL_CTX_new(TLS_method());
    context->is_parent   = 1;

    /* We, as parent context, may ignore data */
    context->sc.ignore_data = (int (*)(struct us_socket_t*))ssl_ignore_data;

    /* Default options we rely on */
    SSL_CTX_set_read_ahead(context->ssl_context, 1);
    SSL_CTX_set_mode(context->ssl_context, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    // SSL_CTX_set_mode(context->ssl_context, SSL_MODE_ENABLE_PARTIAL_WRITE);

    /* Security options; we as application developers should not have to care about these! */
    SSL_CTX_set_options(context->ssl_context, SSL_OP_NO_SSLv3);
    SSL_CTX_set_options(context->ssl_context, SSL_OP_NO_TLSv1);

    /* The following are helpers. You may easily implement whatever you want by using the native
     * handle directly */

    /* Important option for lowering memory usage, but lowers performance slightly */
    if (options.ssl_prefer_low_memory_usage) {
        SSL_CTX_set_mode(context->ssl_context, SSL_MODE_RELEASE_BUFFERS);
    }

    if (options.passphrase) {
        SSL_CTX_set_default_passwd_cb_userdata(context->ssl_context, (void*)options.passphrase);
        SSL_CTX_set_default_passwd_cb(context->ssl_context, passphrase_cb);
    }

    if (options.cert_file_name) {
        if (SSL_CTX_use_certificate_chain_file(context->ssl_context, options.cert_file_name) != 1) {
            return 0;
        }
    }

    if (options.key_file_name) {
        if (SSL_CTX_use_PrivateKey_file(
                context->ssl_context, options.key_file_name, SSL_FILETYPE_PEM) != 1) {
            return 0;
        }
    }

    if (options.dh_params_file_name) {
        /* Set up ephemeral DH parameters. */
        DH*   dh_2048 = NULL;
        FILE* paramfile;
        paramfile = fopen(options.dh_params_file_name, "r");

        if (paramfile) {
            dh_2048 = PEM_read_DHparams(paramfile, NULL, NULL, NULL);
            fclose(paramfile);
        }
        else {
            return 0;
        }

        if (dh_2048 == NULL) { return 0; }

        if (SSL_CTX_set_tmp_dh(context->ssl_context, dh_2048) != 1) { return 0; }

        /* OWASP Cipher String 'A+'
         * (https://www.owasp.org/index.php/TLS_Cipher_String_Cheat_Sheet)
         */
        if (SSL_CTX_set_cipher_list(context->ssl_context,
                                    "DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-"
                                    "AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256") != 1) {
            return 0;
        }
    }

    return context;
}

void us_internal_ssl_socket_context_free(struct us_internal_ssl_socket_context_t* context)
{
    if (context->is_parent) { SSL_CTX_free(context->ssl_context); }

    us_socket_context_free(0, &context->sc);
}

void* us_internal_ssl_socket_context_ext(struct us_internal_ssl_socket_context_t* context)
{
    return context + 1;
}

/* Per socket functions */
void* us_internal_ssl_socket_get_native_handle(struct us_internal_ssl_socket_t* s)
{
    return s->ssl;
}

int us_internal_ssl_socket_write(struct us_internal_ssl_socket_t* s,
                                 const char*                      data,
                                 int                              length,
                                 int                              msg_more)
{
    if (us_socket_is_closed(0, &s->s) || us_internal_ssl_socket_is_shut_down(s)) { return 0; }

    struct us_internal_ssl_socket_context_t* context =
        (struct us_internal_ssl_socket_context_t*)us_socket_context(0, &s->s);

    struct us_loop_t*     loop          = us_socket_context_loop(0, &context->sc);
    struct loop_ssl_data* loop_ssl_data = (struct loop_ssl_data*)loop->data.ssl_data;

    // it makes literally no sense to touch this here! it should start at 0 and ONLY be set and
    // reset by the on_data function! the way is is now, triggering a write from a read will
    // essentially delete all input data! what we need to do is to check if this ever is
    // non-zero and print a warning

    loop_ssl_data->ssl_read_input_length = 0;

    loop_ssl_data->ssl_socket              = &s->s;
    loop_ssl_data->msg_more                = msg_more;
    loop_ssl_data->last_write_was_msg_more = 0;
    // printf("Calling SSL_write\n");
    int written = SSL_write(s->ssl, data, length);
    // printf("Returning from SSL_write\n");
    loop_ssl_data->msg_more = 0;

    if (loop_ssl_data->last_write_was_msg_more && !msg_more) { us_socket_flush(0, &s->s); }

    if (written > 0) { return written; }
    else {
        int err = SSL_get_error(s->ssl, written);
        if (err == SSL_ERROR_WANT_READ) {
            // here we need to trigger writable event next ssl_read!
            s->ssl_write_wants_read = 1;
        }
        else if (err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL) {
            // these two errors may add to the error queue, which is per thread and must be
            // cleared
            ERR_clear_error();

            // all errors here except for want write are critical and should not happen
        }

        return 0;
    }
}

void shutdown(SSLSocket* socket)
{
    struct us_internal_ssl_socket_context_t* context =
        (struct us_internal_ssl_socket_context_t*)us_socket_context(0, &s->s);
    struct us_loop_t*     loop          = us_socket_context_loop(0, &context->sc);
    struct loop_ssl_data* loop_ssl_data = (struct loop_ssl_data*)loop->data.ssl_data;

    // also makes no sense to touch this here!
    // however the idea is that if THIS socket is not the same as ssl_socket then this data
    // is not for me but this is not correct as it is currently anyways, any data available
    // should be properly reset
    loop_ssl_data->ssl_read_input_length = 0;

    // essentially we need two of these: one for CURRENT CALL and one for CURRENT SOCKET
    // WITH DATA if those match in the BIO function then you may read, if not then you may
    // not read we need ssl_read_socket to be set in on_data and checked in the BIO
    loop_ssl_data->ssl_socket = &s->s;

    loop_ssl_data->msg_more = 0;

    // sets SSL_SENT_SHUTDOWN no matter what (not actually true if error!)
    int ret = SSL_shutdown(s->ssl);
    if (ret == 0) { ret = SSL_shutdown(s->ssl); }

    if (ret < 0) {

        int err = SSL_get_error(s->ssl, ret);
        if (err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL) {
            // clear
            ERR_clear_error();
        }

        shutdown(socket->fd, SHUT_WR);
        close(socket->fd);
    }
}

int main() {}