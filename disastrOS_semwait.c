#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphore.h"
#include "disastrOS_semdescriptor.h"
#include "disastrOS_constants.h"

void internal_semWait(){
  // do stuff :)
  int fd = running->syscall_args[0];  //assegnazione file descriptor

  //Descriptor*  DescriptorList_byFd(ListHead* l, int fd)
  SemDescriptor* fd_sem = (SemDescriptor*)SemDescriptorList_byFd(&running->sem_descriptors, fd);  //verifico la presenza del descrittore nella lista dei descittori del semaforo

  if(!fd_sem) {  //caso in cui il descrittore non è presente nella lista
    printf("[ERROR]: Il file descriptor cercato %d non esiste\n", fd);
    running->syscall_retvalue = DSOS_ESEM_FDES_LIST;
    return;
  }
  Semaphore* sem = fd_sem->semaphore;  //risalgo al semaforo del descrittore: fd_sem

  printf("[WAIT] PID: %d, ha semaforo ID: %d, contatore decrementa a %d\n", disastrOS_getpid(), sem->id, sem->count-1);
  sem->count = sem->count - 1;  //decremento contatore Semaphore associato alla Wait

  SemDescriptorPtr* fd_semPtr = fd_sem->ptr;  //risalgo al puntatore a descittore

  //Sposto il puntatore a descrittore nella lista di Waiting
  if(sem->count < 0) {  //il contatore del semaforo ha valore negativo -> sospendo il task
    SemDescriptorPtr* ret0 = (SemDescriptorPtr*)List_detach(&sem->descriptors, (ListItem *)fd_semPtr);  //rimuovo il puntatore a descrittore: fd_semPtr dalla lista dei descittori del semaforo

    if(!ret0) {  //caso in cui la rimozione del puntatore a descrittore, dalla lista, fallisce
      printf("[ERROR]: Rimozione del puntatore a descrittore fallita!\n");
      running->syscall_retvalue = DSOS_ELIST_DETACH;
      return;
    }

    SemDescriptorPtr* ret1 = (SemDescriptorPtr*)List_insert(&sem->waiting_descriptors, sem->waiting_descriptors.last, (ListItem *)fd_semPtr);  //inserisco il puntatore a descrittore: fd_semPtr nella lista di waiting del semaforo (in coda)

    if(!ret1) {  //caso in cui l'inserimento del puntatore a descrittore, nella lista di waiting, fallisce
      printf("[ERROR]: Inserimento del puntatore a descrittore, nella waiting_list, fallita!\n");
      running->syscall_retvalue = DSOS_ELIST_INSERT;
      return;
    }

    running->status = Waiting;  //imposto lo stato del processo corrente

    //sposto il PCB dalla ready alla waiting list
    PCB* pcb_aux = (PCB*)List_insert(&waiting_list, waiting_list.last, (ListItem *)running);  //inserisco il process control block del processo corrente nella waiting_list (in coda)

    if(!pcb_aux) {
      printf("[ERROR]: Inserimento del processo, nella waiting_list, fallita!\n");
      running->return_value = DSOS_ELIST_INSERT;
      return;
    }

    pcb_aux = (PCB*)List_detach(&ready_list, (ListItem *)ready_list.first);  //rimuovo il process control block del primo processo nella ready_list

    if(!pcb_aux) {
      printf("[ERROR]: Rimozione del processo, dalla ready_list, fallita!\n");
      running->syscall_retvalue= DSOS_ELIST_DETACH;
      return;
    }

    running = pcb_aux;
  }

  running->syscall_retvalue = 0;
}
