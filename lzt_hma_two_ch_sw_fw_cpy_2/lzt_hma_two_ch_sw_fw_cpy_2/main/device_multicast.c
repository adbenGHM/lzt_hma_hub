
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "app_main.h"

#define MULTICAST_LOOPBACK CONFIG_EXAMPLE_LOOPBACK

#define MULTICAST_IPV4_ADDR "224.0.0.0"
#define UDP_PORT 8091
#define MULTICAST_TTL 21

#define LISTEN_ALL_IF   EXAMPLE_MULTICAST_LISTEN_ALL_IF

static const char *TAG = "multicast";
static const char *V4TAG = "mcast-ipv4";


/* Add a socket, either IPV4-only or IPV6 dual mode, to the IPV4
   multicast group */
static int socket_add_ipv4_multicast_group(int sock, bool assign_source_if)
{
    struct ip_mreq imreq = { 0 };
    struct in_addr iaddr = { 0 };
    int err = 0;
    // Configure source interface
#if LISTEN_ALL_IF
    imreq.imr_interface.s_addr = IPADDR_ANY;
#else
    esp_netif_ip_info_t ip_info = { 0 };
    inet_addr_from_ip4addr(&iaddr, &ip_info.ip);
#endif // LISTEN_ALL_IF
    // Configure multicast address to listen to
    err = inet_aton(MULTICAST_IPV4_ADDR, &imreq.imr_multiaddr.s_addr);
    if (err != 1) {
        ESP_LOGE(V4TAG, "Configured IPV4 multicast address '%s' is invalid.", MULTICAST_IPV4_ADDR);
        // Errors in the return value have to be negative
        err = -1;
        goto err;
    }
    ESP_LOGI(TAG, "Configured IPV4 Multicast address %s", inet_ntoa(imreq.imr_multiaddr.s_addr));
    if (!IP_MULTICAST(ntohl(imreq.imr_multiaddr.s_addr))) {
        ESP_LOGW(V4TAG, "Configured IPV4 multicast address '%s' is not a valid multicast address. This will probably not work.", MULTICAST_IPV4_ADDR);
    }

    if (assign_source_if) {
        // Assign the IPv4 multicast source interface, via its IP
        // (only necessary if this socket is IPV4 only)
        err = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &iaddr,
                         sizeof(struct in_addr));
        if (err < 0) {
            ESP_LOGE(V4TAG, "Failed to set IP_MULTICAST_IF. Error %d", errno);
            goto err;
        }
    }

    err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                         &imreq, sizeof(struct ip_mreq));
    if (err < 0) {
        ESP_LOGE(V4TAG, "Failed to set IP_ADD_MEMBERSHIP. Error %d", errno);
        goto err;
    }

 err:
    return err;
}

static int create_multicast_ipv4_socket(void)
{
    struct sockaddr_in saddr = { 0 };
    int sock = -1;
    int err = 0;

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(V4TAG, "Failed to create socket. Error %d", errno);
        return -1;
    }

    // Bind the socket to any address
    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(UDP_PORT);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (err < 0) {
        ESP_LOGE(V4TAG, "Failed to bind socket. Error %d", errno);
        goto err;
    }


    // Assign multicast TTL (set separately from normal interface TTL)
    uint8_t ttl = MULTICAST_TTL;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(uint8_t));
    if (err < 0) {
        ESP_LOGE(V4TAG, "Failed to set IP_MULTICAST_TTL. Error %d", errno);
        goto err;
    }

#if MULTICAST_LOOPBACK
    // select whether multicast traffic should be received by this device, too
    // (if setsockopt() is not called, the default is no)
    uint8_t loopback_val = MULTICAST_LOOPBACK;
    err = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP,
                     &loopback_val, sizeof(uint8_t));
    if (err < 0) {
        ESP_LOGE(V4TAG, "Failed to set IP_MULTICAST_LOOP. Error %d", errno);
        goto err;
    }
#endif

    // this is also a listening socket, so add it to the multicast
    // group for listening...
    err = socket_add_ipv4_multicast_group(sock, true);
    if (err < 0) {
        goto err;
    }

    // All set, socket is configured for sending and receiving
    return sock;

err:
    close(sock);
    return -1;
}
static void is_offline_check_task(void *pvParameters){
    uint8_t flag;
    while(1){
        if(xQueueReceive(device_offlineIndQueue,&flag,portMAX_DELAY)==pdTRUE)
            isOFFLINE = flag;
        printf("FLAG: %d \n\n", isOFFLINE);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
static void mcast_example_task(void *pvParameters)
{
    int sock;
    app_nodeData_t nodeResponse;
    while (1) {
        if(isOFFLINE){
            printf("FLAG FROM TASK: %d \n\n", isOFFLINE);
            sock = create_multicast_ipv4_socket();
            if (sock < 0) {
                ESP_LOGE(TAG, "Failed to create IPv4 multicast socket");
            }

            if (sock < 0) {
                // Nothing to do!
                vTaskDelay(5 / portTICK_PERIOD_MS);
                continue;
            }
            // set destination multicast addresses for sending from these sockets
            struct sockaddr_in sdestv4 = {
                .sin_family = PF_INET,
                .sin_port = htons(UDP_PORT),
            };
            // We know this inet_aton will pass because we did it above already
            inet_aton(MULTICAST_IPV4_ADDR, &sdestv4.sin_addr.s_addr);

            // Loop waiting for UDP received, and sending UDP packets if we don't
            // see any.
            int err = 1;
            while (err > 0 && isOFFLINE) {
                struct timeval tv = {
                    .tv_sec = 2,
                    .tv_usec = 0,
                };
                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(sock, &rfds);

                int s = select(sock + 1, &rfds, NULL, NULL, &tv);
                if (s < 0) {
                    ESP_LOGE(TAG, "Select failed: errno %d", errno);
                    err = -1;
                    break;
                }
                else if (s > 0) {
                    if (FD_ISSET(sock, &rfds)) {
                        // Incoming datagram received
                        char recvbuf[APP_MAX_MQTT_DATA_LENGTH+1];
                        char raddr_name[32] = { 0 };

                        struct sockaddr_in6 raddr; // Large enough for both IPv4 or IPv6
                        socklen_t socklen = sizeof(raddr);
                        int len = recvfrom(sock, recvbuf, sizeof(recvbuf)-1, 0,
                                        (struct sockaddr *)&raddr, &socklen);
                        if (len < 0) {
                            ESP_LOGE(TAG, "multicast recvfrom failed: errno %d", errno);
                            err = -1;
                            break;
                        }

                        // Get the sender's address as a string
                        if (raddr.sin6_family == PF_INET) {
                            inet_ntoa_r(((struct sockaddr_in *)&raddr)->sin_addr.s_addr,
                                        raddr_name, sizeof(raddr_name)-1);
                        }
                        ESP_LOGI(TAG, "received %d bytes from %s:", len, raddr_name);

                        recvbuf[len] = 0; // Null-terminate whatever we received and treat like a string...
                        ESP_LOGI(TAG, "%s", recvbuf);
                        processContolMqttData(recvbuf,strlen(recvbuf));
                    //     static int send_count;
                    // char sendfmt[APP_MAX_MQTT_DATA_LENGTH+1];
                    // char sendbuf[APP_MAX_MQTT_DATA_LENGTH+1];
                    // char addrbuf[32] = {0};
                    // memset(nodeResponse.nodeId, '\0', sizeof(nodeResponse.nodeId));
                    // memset(nodeResponse.data, '\0', sizeof(nodeResponse.data));
                    // memset(sendfmt, '\0', sizeof(sendfmt));
                    // sprintf((char*)sendfmt,"{\"NodeId\" : \"%s\", \"Data\" : %s, \"Type\" : %d}",nodeResponse.nodeId,nodeResponse.data,nodeResponse.pck_type);
                    // len = snprintf(sendbuf, sizeof(sendbuf), sendfmt, send_count++);
                    // if (len > sizeof(sendbuf))
                    // {
                    //     ESP_LOGE(TAG, "Overflowed multicast sendfmt buffer!!");
                    //     send_count = 0;
                    //     err = -1;
                    //     break;
                    //     }

                    //     struct addrinfo hints = {
                    //         .ai_flags = AI_PASSIVE,
                    //         .ai_socktype = SOCK_DGRAM,
                    //     };
                    //     struct addrinfo *res;
                    //     hints.ai_family = AF_INET; // For an IPv4 socket
                    //     int err1 = getaddrinfo(MULTICAST_IPV4_ADDR,
                    //                         NULL,
                    //                         &hints,
                    //                         &res);
                    //     if (err1 < 0) {
                    //         ESP_LOGE(TAG, "getaddrinfo() failed for IPV4 destination address. error: %d", err);
                    //         break;
                    //     }
                    //     if (res == 0) {
                    //         ESP_LOGE(TAG, "getaddrinfo() did not return any addresses");
                    //         break;
                    //     }
                    //     ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(UDP_PORT);
                    //     inet_ntoa_r(((struct sockaddr_in *)res->ai_addr)->sin_addr, addrbuf, sizeof(addrbuf)-1);
                    //     ESP_LOGI(TAG, "Sending to IPV4 multicast address %s:%d...",  addrbuf, UDP_PORT);
                    //     err = sendto(sock, sendbuf, len, 0, res->ai_addr, res->ai_addrlen);
                    //     freeaddrinfo(res);
                    //     if (err < 0) {
                    //         ESP_LOGE(TAG, "IPV4 sendto failed. errno: %d", errno);
                    //         break;
                    //     }
                    //     err = 0;
                    }
                }
                else{
                     if (xQueueReceive(app_nodeResponseQueue, &nodeResponse, portMAX_DELAY) == pdPASS){
                //         static int send_count;
                //         char sendfmt[APP_MAX_MQTT_DATA_LENGTH+1];
                //         char sendbuf[APP_MAX_MQTT_DATA_LENGTH+1];
                //         char addrbuf[32] = {0};
                //         memset(nodeResponse.nodeId, '\0', sizeof(nodeResponse.nodeId));
                //         memset(nodeResponse.data, '\0', sizeof(nodeResponse.data));
                //         memset(sendfmt, '\0', sizeof(sendfmt));
                //         sprintf((char*)sendfmt,"{\"NodeId\" : \"%s\", \"Data\" : %s, \"Type\" : %d}",nodeResponse.nodeId,nodeResponse.data,nodeResponse.pck_type);
                //         int len = snprintf(sendbuf, sizeof(sendbuf), sendfmt, send_count++);
                //         printf("\r\n%s\r\n",sendbuf);
                //         if (len > sizeof(sendbuf))
                //         {
                //             ESP_LOGE(TAG, "Overflowed multicast sendfmt buffer!!");
                //             send_count = 0;
                //             err = -1;
                //             break;
                //             }

                //             struct addrinfo hints = {
                //                 .ai_flags = AI_PASSIVE,
                //                 .ai_socktype = SOCK_DGRAM,
                //             };
                //             struct addrinfo *res;
                //             hints.ai_family = AF_INET; // For an IPv4 socket
                //             int err1 = getaddrinfo(MULTICAST_IPV4_ADDR,
                //                                 NULL,
                //                                 &hints,
                //                                 &res);
                //             if (err1 < 0) {
                //                 ESP_LOGE(TAG, "getaddrinfo() failed for IPV4 destination address. error: %d", err);
                //                 break;
                //             }
                //             if (res == 0) {
                //                 ESP_LOGE(TAG, "getaddrinfo() did not return any addresses");
                //                 break;
                //             }
                //             ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(UDP_PORT);
                //             inet_ntoa_r(((struct sockaddr_in *)res->ai_addr)->sin_addr, addrbuf, sizeof(addrbuf)-1);
                //             ESP_LOGI(TAG, "Sending to IPV4 multicast address %s:%d...",  addrbuf, UDP_PORT);
                //             //err = sendto(sock, sendbuf, len, 0, res->ai_addr, res->ai_addrlen);
                //             freeaddrinfo(res);
                //             if (err < 0) {
                //                 ESP_LOGE(TAG, "IPV4 sendto failed. errno: %d", errno);
                //                 break;
                //             }
                //             err = 0;
                     }
                 }
            }

            //ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void device_multicastInit(void)
{
    device_offlineIndQueue = xQueueCreate(2, sizeof(uint8_t));
    //xTaskCreate(&is_offline_check_task, "is_offline_check_task", 4096, NULL, 5, NULL);
    xTaskCreate(&mcast_example_task, "mcast_task", 5120, NULL, 5, NULL);
    
}
