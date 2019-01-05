#include "scheduling_simulator.h"

int task_count = 0;

ucontext_t main_ctx;
ucontext_t shell_ctx;
ucontext_t simulator_ctx;
ucontext_t terminate_ctx;
ucontext_t *start;
ucontext_t *end;

struct itimerval timer;//time signal.
struct tasklist *H_ready_head, *H_waiting_head, *terminated_head;//high priority Q.
struct tasklist *L_ready_head, *L_waiting_head;//low priority Q.
char quantum[2]= {};
char task[10]= {};
char priority[2]= {};
struct tasklist *for_remove_temp =NULL;

//It's weird but I really need 2 versions of delete.

void set_timer(int sec, int usec)
{
    timer.it_value.tv_sec = sec; //延时时长
    timer.it_value.tv_usec = usec;
    timer.it_interval.tv_sec = 0; //计时间隔=0,只会延时，不会定时
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
        fprintf(stderr, "\nWrong setitimer.\n");
        exit(EXIT_FAILURE);
    }
}

void hw_suspend(int msec_10)
{
    set_timer(0,0);

    if (H_ready_head!= NULL && H_ready_head->state == TASK_RUNNING) {
        time_countdown(H_ready_head, msec_10);
        H_ready_head->state = TASK_WAITING;
        push(&H_waiting_head, pop(&H_ready_head));
    }

    if (H_ready_head== NULL && L_ready_head!=NULL && L_ready_head->state == TASK_RUNNING) {
        time_countdown(L_ready_head, msec_10);
        L_ready_head->state = TASK_WAITING;
        push(&L_waiting_head, pop(&L_ready_head));
    }

    start = end;
    end = &simulator_ctx;
    swapcontext(start, end);
    return;
}

void hw_wakeup_pid(int pid)
{
    //task5沒有跑出"Mom(task5): wake up pid %d~\n"，但是其實他是有跑的!!!!!
    //ctrlz跳出後可以發現其實task3有被喚醒，再執行一次start，暫停後用ps看可以發現其實task3有被叫醒
    set_timer(0,0);

    // struct tasklist *H_wait_ptr = H_waiting_head;
    // struct tasklist *L_wait_ptr = L_waiting_head;
    // while (H_wait_ptr != NULL) {
    //     if (find_node(H_waiting_head, pid)) {
    //         H_wait_ptr=find_node(H_wait_ptr, pid);
    //         H_wait_ptr->state = TASK_READY;
    //         push(&H_ready_head, for_remove_delete(&H_waiting_head, H_wait_ptr));
    //     }
    // }

    // while (L_wait_ptr != NULL) {
    //     if (find_node(L_waiting_head, pid)) {
    //         L_wait_ptr=find_node(L_wait_ptr, pid);
    //         L_wait_ptr->state = TASK_READY;
    //         push(&L_ready_head, for_remove_delete(&L_waiting_head, L_wait_ptr));
    //     }
    // }
    //*********************************************************************************//
    struct tasklist *temp_H = H_waiting_head;
    struct tasklist *temp_L = L_waiting_head;
    struct tasklist *wake_H = NULL;
    struct tasklist *wake_L = NULL;

    while (temp_H != NULL && temp_H->next != NULL) {//find until the last one.
        if (temp_H->pid==pid && temp_H->state == TASK_WAITING) {//if the same.
            wake_H = temp_H;
            temp_H = temp_H->next;
            wake_H->state = TASK_READY;
            printf("wake up:pid_%d name_%s.\n", wake_H->pid, wake_H->name);
            push(&H_ready_head, delete(&H_waiting_head, wake_H));
            break;
        } else {
            temp_H = temp_H->next;
        }
    }

    while (temp_L != NULL && temp_L->next != NULL) {//find until the last one.
        if (temp_L->pid==pid && temp_H->state == TASK_WAITING) {//if the same.
            wake_L = temp_L;
            temp_L = temp_L->next;
            wake_L->state = TASK_READY;
            printf("wake up:pid_%d name_%s.\n", wake_L->pid, wake_L->name);
            push(&L_ready_head, delete(&L_waiting_head, wake_L));
            break;
        } else {
            temp_L = temp_L->next;
        }
    }

    //*********************************************************************************//

    // struct tasklist *H_wait_ptr = H_waiting_head;
    // struct tasklist *L_wait_ptr = L_waiting_head;

    // while(H_wait_ptr!= NULL && H_wait_ptr->next != NULL) {
    //     if(H_wait_ptr->pid == pid) {
    //         printf("wake up:pid_%d name_%s.\n", H_wait_ptr->pid, H_wait_ptr->name);
    //         H_wait_ptr->state = TASK_READY;
    //         push(&H_ready_head, for_remove_delete(&H_waiting_head, H_wait_ptr));
    //     } else {
    //         H_wait_ptr = H_wait_ptr->next;
    //     }
    // }

    // while(L_wait_ptr!= NULL && L_wait_ptr->next != NULL) {
    //     if(L_wait_ptr->pid == pid) {
    //         printf("wake up:pid_%d name_%s.\n", L_wait_ptr->pid, L_wait_ptr->name);
    //         L_wait_ptr->state = TASK_READY;
    //         push(&L_wait_ptr, for_remove_delete(&H_waiting_head, L_wait_ptr));
    //     } else {
    //         L_wait_ptr = L_wait_ptr->next;
    //     }
    // }

    start = end;
    end = &simulator_ctx;
    swapcontext(start, end);
    return;
}

int hw_wakeup_taskname(char *task_name)
{
    set_timer(0,0);
    int count = 0;
    struct tasklist *temp_H = H_waiting_head;
    struct tasklist *temp_L = L_waiting_head;
    struct tasklist *wake_H = NULL;
    struct tasklist *wake_L = NULL;

    while (temp_H != NULL) {//find until the last one.
        if (strcmp(temp_H->name, task_name)==0) {//if the same.
            wake_H = temp_H;
            temp_H = temp_H->next;
            wake_H->state = TASK_READY;
            push(&H_ready_head, delete(&H_waiting_head, wake_H));
            count++;
        } else {
            temp_H = temp_H->next;
        }
    }

    while (temp_L != NULL) {
        if (strcmp(temp_L->name, task_name)==0) {
            wake_L = temp_L;
            temp_L = temp_L->next;
            wake_L->state = TASK_READY;
            push(&L_ready_head, delete(&L_waiting_head, wake_L));
            count++;
        } else {
            temp_L = temp_L->next;
        }
    }
    start = end;
    end = &simulator_ctx;
    swapcontext(start, end);
    return count;
}



int hw_task_create(char *task_name)
{
    struct tasklist *new_task =
        (struct tasklist *)malloc(sizeof(struct tasklist));
    new_task->context.uc_link = &terminate_ctx;
    new_task->context.uc_stack.ss_sp = malloc(4096);
    new_task->context.uc_stack.ss_size = 4096;
    new_task->context.uc_stack.ss_flags = 0;

    getcontext(&new_task->context);

    strcpy(new_task->name, task_name);
    new_task->state = TASK_READY;

    if (quantum != NULL && quantum[0] == 'L') {
        new_task->quantum = 20;
    }  else {
        new_task->quantum = 10;
    }

    if (priority != NULL && priority[0] == 'H') {
        new_task->prioritynum = 1;
    }  else { //L and default value.
        new_task->prioritynum = 0;
    }

    new_task->queueingtime = 0;
    new_task->wakeup.tv_sec = 0;
    new_task->wakeup.tv_nsec = 0;
    new_task->pid = ++task_count;
    printf("PRODUCE:%s pid:%d state:%s timequan:%s priority:%s\n", new_task->name, new_task->pid, enum_string(new_task), quan_string(new_task), priority_string(new_task));//test.

    if (task_name[4]=='1') {//task1
        makecontext(&(new_task->context), task1, 0);
    } else if (task_name[4]=='2') {//task2
        makecontext(&(new_task->context), task2, 0);
    } else if (task_name[4]=='3') {//task3
        makecontext(&(new_task->context), task3, 0);
    } else if (task_name[4]=='4') {//task4
        makecontext(&(new_task->context), task4, 0);
    } else if (task_name[4]=='5') {//task5
        makecontext(&(new_task->context), task5, 0);
    } else if (task_name[4]=='6') {//task6
        makecontext(&(new_task->context), task6, 0);
    } else {
        return -1;// if there is no function named task_name
    }

    new_task->next = NULL;
    if(priority != NULL && priority[0]=='H') {
        push(&H_ready_head, new_task);
    } else { //default.
        push(&L_ready_head, new_task);
    }

    return new_task->pid; // the pid of created task name
}

int is_empty(struct tasklist *head)
{
    return head == NULL;
}

struct tasklist *lastone(struct tasklist *head)
{
    struct tasklist *final = head;
    while (final != NULL && final->next != NULL) {
        final = final->next;
    }
    return final;
}

void push(struct tasklist **head, struct tasklist *item)
{
    struct tasklist *last = lastone(*head);
    if (last != NULL) {
        item->next = last->next;
        last->next = item;
    } else {
        *head = item;
    }
}

struct tasklist *delete(struct tasklist **head, struct tasklist *item)
{
    if (*head == item) {
        *head = (*head)->next;
    } else {
        struct tasklist *front = *head;
        while (front->next != item);
        front->next = item->next;
    }

    item->next = NULL;
    return item;
}

struct tasklist *pop(struct tasklist **head)
{
    return delete(head, *head);
}

struct tasklist *for_remove_delete(struct tasklist **head, struct tasklist *item)
{
    // Store head node
    //struct tasklist * temp = (struct tasklist *)malloc(sizeof(struct tasklist));
    struct tasklist *prev;
    for_remove_temp = *head;
    // If head node itself holds the key to be deleted
    if (for_remove_temp != NULL && for_remove_temp == item) {
        *head = for_remove_temp->next;   // Changed head               // free old head
        //free(temp);  // Free memory
    }

    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (for_remove_temp != NULL && for_remove_temp != item) {
        prev = for_remove_temp;
        for_remove_temp = for_remove_temp->next;

        // if (for_remove_temp == lastone(*head)) { //test!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //     item->next = NULL;

        // }
    }

    // If key was not present in linked list
    if (for_remove_temp == NULL) return NULL;

    // Unlink the node from linked list
    prev->next = for_remove_temp->next;

    //free(temp);  // Free memory
    return item;
}

void remove_node(int pid)
{
    if (find_node(H_ready_head, pid)) {//if find.
        for_remove_delete(&H_ready_head, find_node(H_ready_head, pid));
    } else if (find_node(H_waiting_head, pid)) {
        for_remove_delete(&H_waiting_head, find_node(H_waiting_head, pid));
    } else if (find_node(terminated_head, pid)) {
        for_remove_delete(&terminated_head, find_node(terminated_head, pid));
    } else if (find_node(L_ready_head, pid)) {
        for_remove_delete(&L_ready_head, find_node(L_ready_head, pid));
    } else if (find_node(L_waiting_head, pid)) {
        for_remove_delete(&L_waiting_head, find_node(L_waiting_head, pid));
    } else {
        printf("can't find pid\n");
    }

    // if(lastone(L_ready_head))//test
    // {
    // printf("last id %d",lastone(L_ready_head)->pid);
    // }
}

char *priority_string(struct tasklist *node)
{
    if(node->prioritynum==1)
        return "H";
    else
        return "L";
}

struct tasklist *find_node(struct tasklist *head, int pid)
{
    // struct tasklist *temp = head;
    // while (temp != NULL && temp->pid != pid) {
    //     temp = temp->next;
    // }
    // return temp; //find the pid!

    while (head != NULL && head->pid != pid) {
        head = head->next;
    }
    return head;
}

char *enum_string(struct tasklist *node)
{
    switch (node->state) {
    case TASK_RUNNING:
        return "TASK_RUNNING";
    case TASK_READY:
        return "TASK_READY";
    case TASK_WAITING:
        return "TASK_WAITING";
    case TASK_TERMINATED:
        return "TASK_TERMINATED";
    default:
        return NULL;
    }
}

char *quan_string(struct tasklist *node)
{
    if(node->quantum==20)
        return "L";
    else
        return "S";
}

void time_countdown(struct tasklist *head, int msec_10)
{
    clock_gettime(CLOCK_MONOTONIC_RAW, &head->wakeup);
    while (msec_10 >= 100) {
        head->wakeup.tv_sec += 1;
        msec_10 -= 100;
    }
    head->wakeup.tv_nsec += 10000000 * msec_10;//10ms=10^-2 by 10^-7 times
    while (head->wakeup.tv_nsec >= 1000000000) {
        head->wakeup.tv_sec += 1;
        head->wakeup.tv_nsec -= 1000000000;
    }

}

void shell()
{
    while (1) {

        set_timer(0,0);

        quantum[0]= 'S';//default
        priority[0]= 'L';//default
        struct tasklist *H_temp_ready = H_ready_head;//for ps.
        struct tasklist *H_temp_waiting = H_waiting_head;
        struct tasklist *temp_terminated = terminated_head;
        struct tasklist *L_temp_ready = L_ready_head;
        struct tasklist *L_temp_waiting = L_waiting_head;
        printf("$ ");
        char input[100];
        fgets(input, 100, stdin);
        if(input[0] == 'a') {//add
            sscanf(input, "add %s -t %s -p %s", task, quantum, priority);

            //if(!strncmp(task, "task1", 5)||!strncmp(task, "task2", 5)||!strncmp(task, "task3", 5)||!strncmp(task, "task4", 5)|| !strncmp(task, "task5", 5)|| !strncmp(task, "task6", 5))
            if((strlen(task)==5) && (task[4]=='1'||task[4]=='2'||task[4]=='3'||task[4]=='4'||task[4]=='5'||task[4]=='6'))
                hw_task_create(task);
            else
                setcontext(&shell_ctx);
        } else if(input[0] == 'r') {//remove
            int pid;
            sscanf(input, "remove %d", &pid);
            remove_node(pid);
        } else if(input[0] == 's') {//start
            puts("simulating...");
            start = &shell_ctx;
            end = &simulator_ctx;
            swapcontext(start, end);
        } else if(input[0] == 'p') {//ps
            if (H_temp_ready == NULL && H_temp_waiting == NULL && temp_terminated == NULL && L_temp_ready == NULL && L_temp_waiting == NULL) {
                puts("There is nothing!");
            } else {
                puts("Pid  Name   State               Qtime    Priority    Quantum");
                //printf("Pid	Name	State	Queueingtime	Priority	Quantum\n");
                while (temp_terminated != NULL) {
                    printf("%-5d%-5s  %-15s	%-d	 %-4s        %-4s\n", temp_terminated->pid, temp_terminated->name,
                           enum_string(temp_terminated),
                           temp_terminated->queueingtime, priority_string(temp_terminated), quan_string(temp_terminated));
                    temp_terminated = temp_terminated->next;
                }
                while (H_temp_ready != NULL) {
                    printf("%-5d%-5s  %-15s	%-d	 %-4s        %-4s\n", H_temp_ready->pid, H_temp_ready->name,
                           enum_string(H_temp_ready), H_temp_ready->queueingtime, priority_string(H_temp_ready), quan_string(H_temp_ready));
                    H_temp_ready = H_temp_ready->next;
                }
                while (H_temp_waiting != NULL) {
                    printf("%-5d%-5s  %-15s	%-d	 %-4s        %-4s\n", H_temp_waiting->pid, H_temp_waiting->name,
                           enum_string(H_temp_waiting), H_temp_waiting->queueingtime, priority_string(H_temp_waiting), quan_string(H_temp_waiting));
                    H_temp_waiting = H_temp_waiting->next;
                }
                while (L_temp_ready != NULL) {
                    printf("%-5d%-5s  %-15s	%-d	 %-4s        %-4s\n", L_temp_ready->pid, L_temp_ready->name,
                           enum_string(L_temp_ready), L_temp_ready->queueingtime, priority_string(L_temp_ready), quan_string(L_temp_ready));
                    L_temp_ready = L_temp_ready->next;
                }
                while (L_temp_waiting != NULL) {
                    printf("%-5d%-5s  %-15s	%-d	 %-4s        %-4s\n", L_temp_waiting->pid, L_temp_waiting->name,
                           enum_string(L_temp_waiting), L_temp_waiting->queueingtime, priority_string(L_temp_waiting), quan_string(L_temp_waiting));
                    L_temp_waiting = L_temp_waiting->next;
                }
            }
        }
    }
}


void simulator()
{
    while (1) {
        set_timer(0,0);

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);

        struct tasklist *H_tmp = H_waiting_head;
        struct tasklist *L_tmp = L_waiting_head;
        struct tasklist *wake = NULL;
        // if(H_ready_head==NULL)
        //     printf("H_ready_head is NULL!!!!!\n");//test.
        while (H_tmp != NULL) {
            if (now.tv_sec > H_tmp->wakeup.tv_sec ||
                    (now.tv_sec >= H_tmp->wakeup.tv_sec &&
                     now.tv_nsec >= H_tmp->wakeup.tv_nsec)) {
                wake = H_tmp;
                H_tmp = H_tmp->next;
                wake->state = TASK_READY;
                push(&H_ready_head, delete(&H_waiting_head, wake));//I think delete here should be revised.
            } else {
                H_tmp = H_tmp->next;
            }
        }

        while (L_tmp != NULL) {
            if (now.tv_sec > L_tmp->wakeup.tv_sec ||
                    (now.tv_sec >= L_tmp->wakeup.tv_sec &&
                     now.tv_nsec >= L_tmp->wakeup.tv_nsec)) {
                wake = L_tmp;
                L_tmp = L_tmp->next;
                wake->state = TASK_READY;
                push(&L_ready_head, delete(&L_waiting_head, wake));//I think delete here should be revised.
            } else {
                L_tmp = L_tmp->next;
            }
        }

        if (!is_empty(H_ready_head)) {
            H_ready_head->state = TASK_RUNNING;
            //printf("High running: %s\n", H_ready_head->name);//test
            //printf("HIGH test_name:%s pid:%d state:%s timequan:%d priority:%s\n", H_ready_head->name, H_ready_head->pid, enum_string(H_ready_head), H_ready_head->quantum, priority_string(H_ready_head));
            //test.
            fflush(stdout);
            set_timer(0, H_ready_head->quantum * 1000);
            start = &simulator_ctx;
            end = &H_ready_head->context;
            swapcontext(start, end);
        } else if(is_empty(H_ready_head) && !is_empty(H_waiting_head)) { //test.
            // H_waiting_head->state = TASK_RUNNING;
            // push(&H_ready_head, delete(&H_waiting_head, H_waiting_head));
            // set_timer(0, H_ready_head->quantum * 1000);
            // start = &simulator_ctx;
            // end = &H_ready_head->context;
            // swapcontext(start, end);
        } else if(is_empty(H_ready_head) && is_empty(H_waiting_head) && !is_empty(L_ready_head)) {
            L_ready_head->state = TASK_RUNNING;
            //printf("Low running: %s\n", L_ready_head->name);//test
            //printf("LOW test_name:%s pid:%d state:%s timequan:%d priority:%s\n", L_ready_head->name, L_ready_head->pid, enum_string(L_ready_head), L_ready_head->quantum, priority_string(L_ready_head));
            //test.
            fflush(stdout);
            set_timer(0, L_ready_head->quantum * 1000);
            start = &simulator_ctx;
            end = &L_ready_head->context;
            swapcontext(start, end);
        } else if(is_empty(H_ready_head) && is_empty(H_waiting_head) && is_empty(L_ready_head) && !is_empty(L_waiting_head) ) {//test.
            // L_waiting_head->state = TASK_RUNNING;
            // push(&L_ready_head, delete(&L_waiting_head, L_waiting_head));
            // set_timer(0, L_ready_head->quantum * 1000);
            // start = &simulator_ctx;
            // end = &L_ready_head->context;
            // swapcontext(start, end);
        } else if (is_empty(H_ready_head) && is_empty(H_waiting_head) && is_empty(L_ready_head) && is_empty(L_waiting_head)) {
            start = &simulator_ctx;
            end = &shell_ctx;
            swapcontext(start, end);
        }

    }
}


void terminate()
{
    while (1) {
        set_timer(0,0);
        if (H_ready_head!=NULL && H_ready_head->state == TASK_RUNNING) {

            struct tasklist *H_ready_ptr = H_ready_head->next;
            while (H_ready_ptr != NULL) {
                H_ready_ptr->queueingtime += H_ready_head->quantum;
                H_ready_ptr = H_ready_ptr->next;
            }

            //printf("H terminate: %s\n", H_ready_head->name); //test.
            fflush(stdout);
            H_ready_head->state = TASK_TERMINATED;
            push(&terminated_head, pop(&H_ready_head));
        }

        if (H_ready_head==NULL && L_ready_head!=NULL && L_ready_head->state == TASK_RUNNING) {

            struct tasklist *L_ready_ptr = L_ready_head->next;
            while (L_ready_ptr != NULL) {
                L_ready_ptr->queueingtime += L_ready_head->quantum;
                L_ready_ptr = L_ready_ptr->next;
            }

            //printf("L terminate: %s\n", L_ready_head->name); //test.
            fflush(stdout);
            L_ready_head->state = TASK_TERMINATED;
            push(&terminated_head, pop(&L_ready_head));
        }

        start = end;
        end = &simulator_ctx;
        swapcontext(start, end);
    }
}


void signal_routine(int sig_num)
{
    set_timer(0,0);
    printf("hha\n");
    if (H_ready_head!=NULL && H_ready_head->state == TASK_RUNNING) {

        struct tasklist *H_ready_ptr = H_ready_head->next;
        while (H_ready_ptr != NULL) {
            H_ready_ptr->queueingtime += H_ready_head->quantum;
            H_ready_ptr = H_ready_ptr->next;
        }

        fflush(stdout);
        H_ready_head->state = TASK_READY;
        push(&H_ready_head, pop(&H_ready_head));
    }

    if (H_ready_head==NULL && L_ready_head!=NULL && L_ready_head->state == TASK_RUNNING) {

        struct tasklist *L_ready_ptr = L_ready_head->next;
        while (L_ready_ptr != NULL) {
            L_ready_ptr->queueingtime += L_ready_head->quantum;
            L_ready_ptr = L_ready_ptr->next;
        }

        fflush(stdout);
        L_ready_head->state = TASK_READY;
        push(&L_ready_head, pop(&L_ready_head));
    }
    start = end;
    end = &simulator_ctx;
    swapcontext(start, end);
}

void signal_stop(int sig_num)
{
    set_timer(0,0);
    printf("1\n");
    if (H_ready_head!=NULL && H_ready_head->state == TASK_RUNNING) {

        struct tasklist *H_ready_ptr = H_ready_head->next;
        while (H_ready_ptr != NULL) {
            H_ready_ptr->queueingtime += H_ready_head->quantum;
            H_ready_ptr = H_ready_ptr->next;
        }

        fflush(stdout);
        H_ready_head->state = TASK_READY;
        push(&H_ready_head, pop(&H_ready_head));
    }

    if (H_ready_head==NULL && L_ready_head!=NULL && L_ready_head->state == TASK_RUNNING) {

        struct tasklist *L_ready_ptr = L_ready_head->next;
        while (L_ready_ptr != NULL) {
            L_ready_ptr->queueingtime += L_ready_head->quantum;
            L_ready_ptr = L_ready_ptr->next;
        }

        fflush(stdout);
        L_ready_head->state = TASK_READY;
        push(&L_ready_head, pop(&L_ready_head));
    }
    printf("2\n");
    start = end;
    end = &shell_ctx;
    swapcontext(start, end);
}

int main()
{
    struct sigaction alrm;
    alrm.sa_handler = &signal_routine;
    sigemptyset(&alrm.sa_mask);
    sigaddset(&alrm.sa_mask, SIGALRM);
    sigaddset(&alrm.sa_mask, SIGTSTP);//Ctrl-Z傳送TSTP訊號（SIGTSTP）
    alrm.sa_flags = 0;
    if (sigaction(SIGALRM, &alrm, NULL) < 0)
        return 1;

    struct sigaction stp;
    stp.sa_handler = &signal_stop;
    sigemptyset(&stp.sa_mask);
    sigaddset(&stp.sa_mask, SIGALRM);
    sigaddset(&stp.sa_mask, SIGTSTP);//Ctrl-Z傳送TSTP訊號（SIGTSTP）
    stp.sa_flags = 0;
    if (sigaction(SIGTSTP, &stp, NULL) < 0)
        return 1;


    getcontext(&main_ctx);
    getcontext(&shell_ctx);
    shell_ctx.uc_link = 0;
    shell_ctx.uc_stack.ss_sp = malloc(4096);
    shell_ctx.uc_stack.ss_size = 4096;
    shell_ctx.uc_stack.ss_flags = 0;
    makecontext(&shell_ctx, (void *)&shell, 0);

    getcontext(&simulator_ctx);
    simulator_ctx.uc_link = 0;
    simulator_ctx.uc_stack.ss_sp = malloc(4096);
    simulator_ctx.uc_stack.ss_size = 4096;
    simulator_ctx.uc_stack.ss_flags = 0;
    makecontext(&simulator_ctx, (void *)&simulator, 0);

    getcontext(&terminate_ctx);
    terminate_ctx.uc_link = 0;
    terminate_ctx.uc_stack.ss_sp = malloc(4096);
    terminate_ctx.uc_stack.ss_size = 4096;
    terminate_ctx.uc_stack.ss_flags = 0;
    makecontext(&terminate_ctx, (void *)&terminate, 0);

    swapcontext(&main_ctx, &shell_ctx);
    return 0;
}
