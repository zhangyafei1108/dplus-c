/* Copyright (c) 2006-2015, DNSPod Inc.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1.Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 2.Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
*/

#include "../dplus.h"

#define DP_DES_ID   12
#define DP_DES_KEY  "@o]T<oX/"
#define BUF_SIZE 102400

int main(int argc, char **argv)
{
    struct addrinfo *answer, hint, *curr;
    char ipstr[16];
    int ret, sfd;
    struct timeval time, time2;
    char http_data[BUF_SIZE];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[1]);
        exit(1);
    }

    //init dplus environment
    dp_set_cache_mem(4*1024*1024);
    dp_set_ttl(90);

#ifdef ENTERPRISE_EDITION
    // 设置企业版加密ID和KEY
    dp_set_des_id_key(DP_DES_ID, DP_DES_KEY);
#endif

    dp_env_init();

    bzero(&hint, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    //first
    gettimeofday(&time, NULL);
    ret = dp_getaddrinfo(argv[1], "http", &hint, &answer);
    if (ret != 0) {
        fprintf(stderr, "dp_getaddrinfo: %s\n", gai_strerror(ret));
        dp_env_destroy();
        return 1;
    }

    for (curr = answer; curr != NULL; curr = curr->ai_next) {
        inet_ntop(AF_INET, &(((struct sockaddr_in *)(curr->ai_addr))->sin_addr),
            ipstr, sizeof(ipstr));
        printf("%s\n", ipstr);
    }
    dp_freeaddrinfo(answer);
    gettimeofday(&time2, NULL);
    printf("first time:%lu ms\n\n", (time2.tv_usec - time.tv_usec)/1000);

    //second
    gettimeofday(&time, NULL);
    ret = dp_getaddrinfo(argv[1], "http", &hint, &answer);
    if (ret != 0) {
        fprintf(stderr, "dp_getaddrinfo: %s\n", gai_strerror(ret));
        dp_env_destroy();
        return 1;
    }

    for (curr = answer; curr != NULL; curr = curr->ai_next) {
        inet_ntop(AF_INET, &(((struct sockaddr_in *)(curr->ai_addr))->sin_addr),
            ipstr, 16);
        printf("%s\n", ipstr);
    }
    gettimeofday(&time2, NULL);
    printf("second time:%lu ms\n\n", (time2.tv_usec - time.tv_usec)/1000);

    printf("cache status:\n");
    dp_cache_status();
    printf("\n");

    printf("start http query:%s\n", argv[1]);
    for (curr = answer; curr != NULL; curr = curr->ai_next) {
        sfd = socket(curr->ai_family, curr->ai_socktype,
               curr->ai_protocol);
        if (sfd == -1)
           continue;

        if (connect(sfd, curr->ai_addr, curr->ai_addrlen) != -1)
            break;

        close(sfd);
    }
    //no longer needed
    dp_freeaddrinfo(answer);

    ret = make_request(sfd, argv[1], "/");
    if (ret < 0) {
        printf("make request failed\n");
        close(sfd);
        return -1;
    }

    ret = fetch_response(sfd, http_data, BUF_SIZE);
    if (ret < 0) {
        printf("fetch response failed\n");
        close(sfd);
        return -1;
    }
    close(sfd);

    printf("%s\n", http_data);

    dp_env_destroy();
    return 0;
}
