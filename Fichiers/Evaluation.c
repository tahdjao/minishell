#include "Evaluation.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

int tube[2];

enum { R, W };

int f(int status) {
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return WTERMSIG(status) + 128;
}
void handler(int sig) {
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
  }
}

int evaluer_expr(Expression *e) {
  struct sigaction so;
  int ret, ret1;
  if (e->type == VIDE) {
    return 0;
  }
  int out, in, fd, erro;
  pid_t pid;
  switch (e->type) {
  case SIMPLE:
    pid = fork();
    if (pid == 0) {
      execvp(e->arguments[0], e->arguments);
      perror("execv");
      exit(1);
    }
    int status;
    waitpid(pid, &status, 0);
    return f(status);
    break;
  case SEQUENCE:
    if (e->gauche->type != VIDE) {
      ret = evaluer_expr(e->gauche);
    }
    if (e->droite->type != VIDE) {
      ret1 = evaluer_expr(e->droite);
    }
    break;
  case SEQUENCE_ET:
    ret = evaluer_expr(e->gauche);
    if (ret == 0) {
      ret1 = evaluer_expr(e->droite);
      return ret1;
    } else {
      return 1;
    }
    break;
  case SEQUENCE_OU:
    ret = evaluer_expr(e->gauche);
    if (ret == 1) {
      ret1 = evaluer_expr(e->droite);
      if (ret1 == 0) {
        return 0;
      } else {
        return ret1;
      }
    } else {
      return 0;
    }
    break;
  case PIPE:
    out = dup(STDOUT_FILENO);
    in = dup(STDIN_FILENO);
    ret1 = pipe(tube);
    pid = fork();
    if (e->gauche->type == VIDE || e->droite->type == VIDE) {
      return 1;
    }
    if (pid == 0) {
      dup2(tube[1], STDOUT_FILENO);
      close(tube[0]);
      close(tube[1]);
      evaluer_expr(e->gauche);
      perror("pipe");
      exit(1);
    }
    waitpid(pid, &status, 0);
    dup2(tube[0], STDIN_FILENO);
    close(tube[0]);
    close(tube[1]);
    evaluer_expr(e->droite);
    close(out);
    close(in);
    return ret;
    break;
  case REDIRECTION_O:
    fd = open(e->arguments[0], O_TRUNC | O_WRONLY | O_CREAT, 0666);
    out = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    close(fd);
    evaluer_expr(e->gauche);
    dup2(out, STDOUT_FILENO);
    close(out);
    break;
  case REDIRECTION_I:
    in = dup(STDIN_FILENO);
    fd = open(e->arguments[0], O_RDONLY);
    dup2(fd, STDIN_FILENO);
    close(fd);
    evaluer_expr(e->gauche);
    dup2(in, STDIN_FILENO);
    close(in);
    break;
  case REDIRECTION_E:
    erro = dup(STDERR_FILENO);
    fd = open(e->arguments[0], O_TRUNC | O_WRONLY | O_CREAT, 0666);
    dup2(fd, STDERR_FILENO);
    close(fd);
    evaluer_expr(e->gauche);
    dup2(erro, STDERR_FILENO);
    close(erro);
    break;

  case REDIRECTION_EO:
    out = dup(STDOUT_FILENO);
    erro = dup(STDERR_FILENO);
    fd = open(e->arguments[0], O_TRUNC | O_WRONLY | O_CREAT, 0666);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);
    evaluer_expr(e->gauche);
    dup2(erro, STDERR_FILENO);
    close(erro);
    dup2(out, STDOUT_FILENO);
    close(out);
    break;
  case BG:
    so.sa_handler = handler;
    so.sa_flags = SA_RESTART;
    sigemptyset(&so.sa_mask);
    sigaction(SIGCHLD, &so, NULL);
    if (fork() == 0) {
      evaluer_expr(e->gauche);
      perror("bg");
      exit(1);
    }
    return 0;
    break;
  default:
    return 1;
  }
}