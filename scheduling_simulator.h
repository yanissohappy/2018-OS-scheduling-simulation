#ifndef SCHEDULING_SIMULATOR_H
#define SCHEDULING_SIMULATOR_H

#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "task.h"

enum TASK_STATE {
	TASK_RUNNING,
	TASK_READY,
	TASK_WAITING,
	TASK_TERMINATED
};

struct tasklist {
    char name[100];
    enum TASK_STATE state;
    int quantum;
    int queueingtime;
    void (*func)(void);
    int prioritynum;
    struct timespec wakeup;
    int pid;
    ucontext_t context;
    struct tasklist *next;
};


void set_timer(int sec, int usec);
struct tasklist *lastone(struct tasklist *list);
void push(struct tasklist **list_head, struct tasklist *item);
struct tasklist *delete (struct tasklist **list_head, struct tasklist *item);
struct tasklist *pop(struct tasklist **list_head);
struct tasklist *for_remove_delete(struct tasklist **head, struct tasklist *item);
void hw_suspend(int msec_10);
void hw_wakeup_pid(int pid);
int hw_wakeup_taskname(char *task_name);
int hw_task_create(char *task_name);
void remove_node(int pid);
struct tasklist *find_node(struct tasklist *head, int pid);
void time_countdown(struct tasklist *head, int msec_10);
char *priority_string(struct tasklist *node);
char *enum_string(struct tasklist *node);
char *quan_string(struct tasklist *node);
void shell();
void simulator();
void terminate();
void signal_routine(int sig_num);
void signal_stop(int sig_num);

#endif
