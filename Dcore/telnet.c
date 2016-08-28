
#include "libhd.h"
#include "shm.h"
#include "define.h"

void teller() {
    while (TEL.runningflag) {
        usleep(200);
        unsigned char pTemp_RecvBuf[MAXRECLEN];

        int sTemp_RecvLength = recv(TEL.destfd, (char *) pTemp_RecvBuf,
                                    MAXRECLEN, 0);

        DDBG("[1]recv = %d\n", sTemp_RecvLength);
        if (sTemp_RecvLength <= 0) {
            close(TEL.acceptSocket);
            close(TEL.destfd);
            break;
        }

        if (sTemp_RecvLength > 0) {
            int n_send_len;

            n_send_len = send(TEL.acceptSocket, pTemp_RecvBuf, sTemp_RecvLength, 0);
        }
    }
}

void *redirector(void *args) {
    while (TEL.runningflag) {
        usleep(200);
        unsigned char pTemp_RecvBuf[MAXRECLEN];

        int sTemp_RecvLength = recv(TEL.acceptSocket, (char *) pTemp_RecvBuf,
                                    MAXRECLEN, 0);

        DDBG("[2]recv = %d\n", sTemp_RecvLength);
        if (sTemp_RecvLength <= 0) {
            close(TEL.acceptSocket);
            close(TEL.destfd);
            break;
        }

        if (sTemp_RecvLength > 0) {
            int n_send_len;

            n_send_len = send(TEL.destfd, pTemp_RecvBuf, sTemp_RecvLength, 0);
        }
    }
    return NULL;
}

void telstart(void) {
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 256000);
    while (pthread_create(&TEL.threadkey, &attr, TEL.server, NULL) != 0)
        sleep(1);
}

//通讯线程
void *telserver(void *args) {
    memset(&TEL.Local_addr, 0, sizeof(struct sockaddr_in));
    TEL.Local_addr.sin_family = AF_INET;
    TEL.Local_addr.sin_port = htons(2025);
    TEL.Local_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(TEL.Local_addr.sin_zero), 8);

    if ((TEL.lsnSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        DDBG("建立本地监听socket时失败。");
    if ((TEL.destfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        DDBG("建立远方socket时失败。");

    int bindret = 1;

    if (bind(TEL.lsnSocket, (struct sockaddr *) &TEL.Local_addr, sizeof(struct sockaddr)) < 0)
        bindret = 0;
    if (listen(TEL.lsnSocket, 1) < 0)
        bindret = 0;
    if (bindret != 1) {
        DDBG("绑定或者监听socket时失败。");
        exit(1);
    }

    while (1) {
        struct timeval tv;

        socklen_t sin_size = sizeof(struct sockaddr_in);

        tv.tv_sec = 1, tv.tv_usec = 0;
        TEL.runningflag = 0;

        FD_ZERO(&TEL.readfd);
        FD_SET(TEL.lsnSocket, &TEL.readfd);

        int conn = select(TEL.lsnSocket + 1, &TEL.readfd, NULL, NULL, &tv);

        if (conn == 0)
            continue;

        TEL.acceptSocket = accept(TEL.lsnSocket, (struct sockaddr *) &TEL.Svr_Addr, &sin_size);
        if (TEL.acceptSocket < 0) {
            DDBG("转发线程：当接受远方连接请求时失败。");
            exit(1);
        }

        TEL.dest_addr.sin_family = AF_INET;
        TEL.dest_addr.sin_port = htons(2404);
        TEL.dest_addr.sin_addr.s_addr = inet_addr(TEL.ip);

        if (-1 == connect(TEL.destfd, (struct sockaddr *) &TEL.dest_addr, sizeof(struct sockaddr))) {
            perror("connect error\n");
            exit(0);
        }
        TEL.runningflag = 1;

        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, 256000);
        while (pthread_create(&TEL.threadkey, &attr, redirector, NULL) != 0)
            sleep(1);

        teller();

    }
    return NULL;
}
