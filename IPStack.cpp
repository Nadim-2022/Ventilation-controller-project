//
// Created by Keijo Länsikunnas on 12.2.2024.
//
#include <cstring>
#include "pico/time.h"

#include "IPStack.h"


// To remove Pico example debugging functions during refactoring
#define DEBUG_printf(x, ...) {}
#define DUMP_BYTES(A, B) {}


IPStack::IPStack() : count{0}, wr{0}, rd{0}, connected{false} {

}

int IPStack::connect(uint32_t hostname, int port) {
    return true;
}

int IPStack::connect(const char *hostname, int port) {
    // check if the hostname requires DNS resolution
    if (!ip4addr_aton(hostname, &remote_addr)) {
        // dns not implemented yet
        return 0;
    }
    // open a socket connection
    DEBUG_printf("Connecting to %s port %u\n", ip4addr_ntoa(remote_addr), port);
    tcp_pcb = std::unique_ptr<struct tcp_pcb>(tcp_new_ip_type(IP_GET_TYPE(remote_addr)));
    if (!tcp_pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    tcp_arg(tcp_pcb.get(), this);
    tcp_poll(tcp_pcb.get(), IPStack::tcp_client_poll, POLL_TIME_S * 2);
    tcp_sent(tcp_pcb.get(), IPStack::tcp_client_sent);
    tcp_recv(tcp_pcb.get(), IPStack::tcp_client_recv);
    tcp_err(tcp_pcb.get(), IPStack::tcp_client_err);

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(tcp_pcb.get(), &remote_addr, port, IPStack::tcp_client_connected);
    cyw43_arch_lwip_end();

    return err == ERR_OK;
}

/** Function prototype for tcp sent callback functions. Called when sent data has
 * been acknowledged by the remote side. Use it to free corresponding resources.
 * This also means that the pcb has now space available to send new data.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb for which data has been acknowledged
 * @param len The amount of bytes acknowledged
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
err_t IPStack::tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    //auto state = static_cast<IPStack *>(arg);
    DEBUG_printf("tcp_client_sent %u\n", len);

    return ERR_OK;
}

/** Function prototype for tcp connected callback functions. Called when a pcb
 * is connected to the remote side after initiating a connection attempt by
 * calling tcp_connect().
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which is connected
 * @param err An unused error code, always ERR_OK currently ;-) @todo!
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 *
 * @note When a connection attempt fails, the error callback is currently called!
 */
err_t IPStack::tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    auto state = static_cast<IPStack *>(arg);
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
    }
    state->connected = true;

    return ERR_OK;
}

/** Function prototype for tcp poll callback functions. Called periodically as
 * specified by @see tcp_poll.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb tcp pcb
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
err_t IPStack::tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    //auto state = static_cast<IPStack *>(arg);
    DEBUG_printf("tcp_client_poll\n");
    return ERR_OK;
}

/** Function prototype for tcp error callback functions. Called when the pcb
 * receives a RST or is unexpectedly closed for any other reason.
 *
 * @note The corresponding pcb is already freed when this callback is called!
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param err Error code to indicate why the pcb has been closed
 *            ERR_ABRT: aborted through tcp_abort or by a TCP timer
 *            ERR_RST: the connection was reset by the remote host
 */
void IPStack::tcp_client_err(void *arg, err_t err) {
    //auto state = static_cast<IPStack *>(arg);
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err %d\n", err);
        //state->tcp_result(err);
    }
}

/** Function prototype for tcp receive callback functions. Called when data has
 * been received.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which received data
 * @param p The received data (or NULL when the connection has been closed!)
 * @param err An error code if there has been an error receiving
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
err_t IPStack::tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    auto state = static_cast<IPStack *>(arg);
    if (!p) {
        // connection has been closed - do we need to react to this somehow?
        return ERR_OK;
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
#if 0
        DEBUG_printf("recv %d err %d\n", p->tot_len, err);
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            DUMP_BYTES(q->payload, q->len);
        }
#endif
        // Receive the buffer
        uint16_t available = BUF_SIZE - state->count;
        uint16_t bytes_to_copy = available > p->tot_len ? p->tot_len : available;
        uint16_t wr_end = state->wr + bytes_to_copy;
        if (wr_end > BUF_SIZE) {
            // need to copy in two parts
            uint16_t first_copy = BUF_SIZE - state->wr; // calculate the size of first part to copy
            if (first_copy) { //
                bytes_to_copy -= pbuf_copy_partial(p, state->buffer + state->wr, first_copy, 0);
            }
            // start from beginning
            state->wr = pbuf_copy_partial(p, state->buffer + state->wr, bytes_to_copy, first_copy);
        } else {
            state->wr += pbuf_copy_partial(p, state->buffer + state->wr, bytes_to_copy, 0);
            state->wr %= BUF_SIZE; // wrap over (should happen only when wr==BUFSIZE, other cases are handled in two part copy)
        }
        state->count += bytes_to_copy;
        tcp_recved(tpcb, p->tot_len);
        if(bytes_to_copy < p->tot_len) {
            state->dropped += p->tot_len - bytes_to_copy;
        }
    }
    pbuf_free(p); // can we omit this call after partial copy to save the buffer for copying the rest later?

    return ERR_OK;
}


int IPStack::read(unsigned char *buffer, int len, int timeout) {
    // is it possible to call with zero timeout?
    auto to = make_timeout_time_ms(timeout);
    do {
        cyw43_arch_poll();
    } while (count < len && !time_reached(to));

    int bytes_to_copy = count < len ? count : len;
    if (bytes_to_copy) {
        uint16_t rd_end = rd + bytes_to_copy;
        if (rd_end > BUF_SIZE) {
            // need to copy in two parts
            uint16_t first_copy = BUF_SIZE - rd; // calculate the size of the first part to copy
            if (first_copy) { //
                std::memcpy(buffer, this->buffer + rd, first_copy);
                bytes_to_copy -= first_copy;
            }
            // start from beginning
            std::memcpy(buffer + first_copy, this->buffer, bytes_to_copy);
            rd = bytes_to_copy;
        } else {
            std::memcpy(buffer, this->buffer + rd, bytes_to_copy);
            rd = (rd + bytes_to_copy) % BUF_SIZE;
        }
        count -= bytes_to_copy;
    }
    // return count or status??
    return bytes_to_copy;
}

int IPStack::write(unsigned char *buffer, int len, int timeout) {
    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();

    err_t err = tcp_write(tcp_pcb.get(), buffer, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to write data %d\n", err);
        return -1;
    }
    // headers suggest that this should be called to make sure that data is sent right away
    // however there is TCB_WRITE_FLAG_MORE that possibly indicates the same thing??
    if (tcp_output(tcp_pcb.get()) != ERR_OK) {
        // failed! What should I do now?
    }

    cyw43_arch_lwip_end();

    return ERR_OK;
}

int IPStack::disconnect() {
    cyw43_arch_lwip_begin();

    err_t err = ERR_OK;
    if (tcp_pcb != nullptr) {
        tcp_arg(tcp_pcb.get(), NULL);
        tcp_poll(tcp_pcb.get(), NULL, 0);
        tcp_sent(tcp_pcb.get(), NULL);
        tcp_recv(tcp_pcb.get(), NULL);
        tcp_err(tcp_pcb.get(), NULL);
        err = tcp_close(tcp_pcb.get());
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(tcp_pcb.get());
            err = ERR_ABRT;
        }
        tcp_pcb = nullptr;
    }
    cyw43_arch_lwip_end();
    return err;
}

