#define _POSIX_C_SOURCE 200809L
#include "async/async.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <unistd.h>

static bool executable_exists(const char *path) {
  return access(path, X_OK) == 0;
}

static AsyncStatus read_pipe_to_buffer(int fd, uint8_t *buffer,
                                       size_t buffer_size, size_t *write_pos,
                                       size_t *read_pos, size_t *count) {
  int bytes_available = 0;
  if (ioctl(fd, FIONREAD, &bytes_available) < 0) {
    return ASYNC_ERROR;
  }

  if (bytes_available == 0) {
    return ASYNC_PENDING;
  }

  size_t bytes_to_read =
      bytes_available < buffer_size ? bytes_available : buffer_size;
  ssize_t bytes_read = read(fd, buffer, bytes_to_read);

  if (bytes_read > 0) {
    for (ssize_t i = 0; i < bytes_read; i++) {
      if (*count >= buffer_size) {
        *read_pos = (*read_pos + 1) % buffer_size;
      } else {
        buffer[*write_pos] = ((uint8_t *)buffer)[i];
        *write_pos = (*write_pos + 1) % buffer_size;
        (*count)++;
      }
    }
  }

  return ASYNC_PENDING;
}

AsyncResult platform_process_spawn(AsyncResult *result, const char *executable,
                                   const char **args,
                                   const ProcessSpawnOptions *options) {
  (void)options;

  if (!executable_exists(executable)) {
    result->status = ASYNC_ERROR;
    result->error =
        fun_error_result(ERROR_CODE_PROCESS_NOT_FOUND, "Executable not found");
    return *result;
  }

  int stdout_pipe[2];
  int stderr_pipe[2];

  if (pipe(stdout_pipe) < 0) {
    result->status = ASYNC_ERROR;
    result->error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
                                     "Failed to create stdout pipe");
    return *result;
  }

  if (pipe(stderr_pipe) < 0) {
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    result->status = ASYNC_ERROR;
    result->error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
                                     "Failed to create stderr pipe");
    return *result;
  }

  pid_t pid = fork();

  if (pid < 0) {
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
    result->status = ASYNC_ERROR;
    result->error =
        fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED, "Fork failed");
    return *result;
  }

  if (pid == 0) {
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    dup2(stdout_pipe[1], STDOUT_FILENO);
    dup2(stderr_pipe[1], STDERR_FILENO);

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    if (args != NULL) {
      execvp(executable, (char *const *)args);
    } else {
      const char *argv[] = {executable, NULL};
      execvp(executable, (char *const *)argv);
    }

    _exit(127);
  }

  close(stdout_pipe[1]);
  close(stderr_pipe[1]);

  int flags_stdout = fcntl(stdout_pipe[0], F_GETFL, 0);
  fcntl(stdout_pipe[0], F_SETFL, flags_stdout | O_NONBLOCK);

  int flags_stderr = fcntl(stderr_pipe[0], F_GETFL, 0);
  fcntl(stderr_pipe[0], F_SETFL, flags_stderr | O_NONBLOCK);

  result->process.handle = (void *)(intptr_t)pid;
  result->process.stdout_pipe = (void *)(intptr_t)stdout_pipe[0];
  result->process.stderr_pipe = (void *)(intptr_t)stderr_pipe[0];
  result->process.is_running = 1;
  result->process.platform_data[0] = 0;

  return *result;
}

AsyncStatus platform_process_poll(AsyncResult *result) {
  if (result == NULL || result->process.handle == NULL) {
    return ASYNC_ERROR;
  }

  int status = 0;
  pid_t pid = (pid_t)(intptr_t)result->process.handle;

  int ret = waitpid(pid, &status, WNOHANG);

  if (ret < 0) {
    return ASYNC_ERROR;
  }

  if (ret > 0) {
    result->process.is_running = 0;

    if (WIFEXITED(status)) {
      result->process.exit_code = WEXITSTATUS(status);
    } else {
      result->process.exit_code = -1;
    }

    result->status = ASYNC_COMPLETED;

    int stdout_fd = (int)(intptr_t)result->process.stdout_pipe;
    int stderr_fd = (int)(intptr_t)result->process.stderr_pipe;

    if (stdout_fd >= 0) {
      read_pipe_to_buffer(
          stdout_fd, result->process.stdout_buffer, PROCESS_STDOUT_BUFFER_SIZE,
          &result->process.stdout_write_pos, &result->process.stdout_read_pos,
          &result->process.stdout_count);
      close(stdout_fd);
      result->process.stdout_pipe = NULL;
    }

    if (stderr_fd >= 0) {
      read_pipe_to_buffer(
          stderr_fd, result->process.stderr_buffer, PROCESS_STDERR_BUFFER_SIZE,
          &result->process.stderr_write_pos, &result->process.stderr_read_pos,
          &result->process.stderr_count);
      close(stderr_fd);
      result->process.stderr_pipe = NULL;
    }

    result->process.handle = NULL;
    return ASYNC_COMPLETED;
  }

  int stdout_fd = (int)(intptr_t)result->process.stdout_pipe;
  int stderr_fd = (int)(intptr_t)result->process.stderr_pipe;

  if (stdout_fd >= 0) {
    read_pipe_to_buffer(
        stdout_fd, result->process.stdout_buffer, PROCESS_STDOUT_BUFFER_SIZE,
        &result->process.stdout_write_pos, &result->process.stdout_read_pos,
        &result->process.stdout_count);
  }

  if (stderr_fd >= 0) {
    read_pipe_to_buffer(
        stderr_fd, result->process.stderr_buffer, PROCESS_STDERR_BUFFER_SIZE,
        &result->process.stderr_write_pos, &result->process.stderr_read_pos,
        &result->process.stderr_count);
  }

  return ASYNC_PENDING;
}

ErrorResult platform_process_terminate(Process *process) {
  if (process == NULL || process->handle == NULL) {
    return ERROR_RESULT_NO_ERROR;
  }

  pid_t pid = (pid_t)(intptr_t)process->handle;

  if (kill(pid, SIGKILL) < 0) {
    if (errno == ESRCH) {
      return ERROR_RESULT_NO_ERROR;
    }
    return fun_error_result(ERROR_CODE_PROCESS_TERMINATE_FAILED,
                            "Failed to terminate process");
  }

  process->is_running = 0;
  return ERROR_RESULT_NO_ERROR;
}

ErrorResult platform_process_free(Process *process) {
  if (process == NULL) {
    return ERROR_RESULT_NULL_POINTER;
  }

  if (process->handle != NULL) {
    pid_t pid = (pid_t)(intptr_t)process->handle;
    int status;
    waitpid(pid, &status, WNOHANG);
    process->handle = NULL;
  }

  if (process->stdout_pipe != NULL) {
    int fd = (int)(intptr_t)process->stdout_pipe;
    if (fd >= 0) {
      close(fd);
    }
    process->stdout_pipe = NULL;
  }

  if (process->stderr_pipe != NULL) {
    int fd = (int)(intptr_t)process->stderr_pipe;
    if (fd >= 0) {
      close(fd);
    }
    process->stderr_pipe = NULL;
  }

  process->is_running = 0;
  return ERROR_RESULT_NO_ERROR;
}
