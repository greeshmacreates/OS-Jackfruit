#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>

#define LOG_DIR "logs"
#define LOG_BUFFER_CAPACITY 16
#define LOG_CHUNK_SIZE 4096
#define CONTAINER_ID_LEN 32

typedef struct {
    char container_id[CONTAINER_ID_LEN];
    size_t length;
    char data[LOG_CHUNK_SIZE];
} log_item_t;

typedef struct {
    log_item_t items[LOG_BUFFER_CAPACITY];
    int head, tail, count;
    int shutting_down;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} bounded_buffer_t;

typedef struct {
    bounded_buffer_t log_buffer;
    pthread_t logger_thread;
} supervisor_ctx_t;

/* ---------------- BUFFER ---------------- */

void buffer_init(bounded_buffer_t *b) {
    memset(b, 0, sizeof(*b));
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->not_empty, NULL);
    pthread_cond_init(&b->not_full, NULL);
}

void buffer_shutdown(bounded_buffer_t *b) {
    pthread_mutex_lock(&b->mutex);
    b->shutting_down = 1;
    pthread_cond_broadcast(&b->not_empty);
    pthread_cond_broadcast(&b->not_full);
    pthread_mutex_unlock(&b->mutex);
}

int bounded_buffer_push(bounded_buffer_t *b, const log_item_t *item) {
    pthread_mutex_lock(&b->mutex);

    while (b->count == LOG_BUFFER_CAPACITY && !b->shutting_down)
        pthread_cond_wait(&b->not_full, &b->mutex);

    if (b->shutting_down) {
        pthread_mutex_unlock(&b->mutex);
        return -1;
    }

    b->items[b->tail] = *item;
    b->tail = (b->tail + 1) % LOG_BUFFER_CAPACITY;
    b->count++;

    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->mutex);
    return 0;
}

int bounded_buffer_pop(bounded_buffer_t *b, log_item_t *item) {
    pthread_mutex_lock(&b->mutex);

    while (b->count == 0 && !b->shutting_down)
        pthread_cond_wait(&b->not_empty, &b->mutex);

    if (b->count == 0 && b->shutting_down) {
        pthread_mutex_unlock(&b->mutex);
        return -1;
    }

    *item = b->items[b->head];
    b->head = (b->head + 1) % LOG_BUFFER_CAPACITY;
    b->count--;

    pthread_cond_signal(&b->not_full);
    pthread_mutex_unlock(&b->mutex);
    return 0;
}

/* ---------------- LOGGING ---------------- */

void *logging_thread(void *arg) {
    supervisor_ctx_t *ctx = (supervisor_ctx_t *)arg;
    log_item_t item;

    mkdir(LOG_DIR, 0777);

    while (1) {
        if (bounded_buffer_pop(&ctx->log_buffer, &item) != 0)
            break;

        char filename[PATH_MAX];
        snprintf(filename, sizeof(filename), LOG_DIR"/%s.log", item.container_id);

        FILE *f = fopen(filename, "a");
        if (f) {
            fwrite(item.data, 1, item.length, f);
            fclose(f);
        }
    }
    return NULL;
}

/* ---------------- CONTAINER ---------------- */

void run_container(const char *id, const char *cmd, supervisor_ctx_t *ctx) {
    int pipefd[2];
    pipe(pipefd);

    pid_t pid = fork();

    if (pid == 0) {
        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
        perror("exec");
        exit(1);
    } else {
        close(pipefd[1]);

        char buffer[LOG_CHUNK_SIZE];
        int n;

        while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            log_item_t item;
            strncpy(item.container_id, id, CONTAINER_ID_LEN);
            memcpy(item.data, buffer, n);
            item.length = n;

            bounded_buffer_push(&ctx->log_buffer, &item);
        }

        wait(NULL);
    }
}

/* ---------------- SUPERVISOR ---------------- */

void run_supervisor() {
    supervisor_ctx_t ctx;

    buffer_init(&ctx.log_buffer);
    pthread_create(&ctx.logger_thread, NULL, logging_thread, &ctx);

    printf("Supervisor running...\n");

    while (1) {
        sleep(1);
    }
}

/* ---------------- MAIN ---------------- */

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage:\n");
        printf("./engine supervisor\n");
        printf("./engine run <id> \"command\"\n");
        return 1;
    }

    supervisor_ctx_t ctx;
    buffer_init(&ctx.log_buffer);
    pthread_create(&ctx.logger_thread, NULL, logging_thread, &ctx);

    if (strcmp(argv[1], "supervisor") == 0) {
        run_supervisor();
    }

    else if (strcmp(argv[1], "run") == 0) {
        if (argc < 4) {
            printf("Usage: ./engine run <id> \"command\"\n");
            return 1;
        }

        run_container(argv[2], argv[3], &ctx);
    }

    buffer_shutdown(&ctx.log_buffer);
    pthread_join(ctx.logger_thread, NULL);

    return 0;
}
