#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_semaphore.h"
#include "disastrOS_semdescriptor.h"
#include "disastrOS_constants.h"

void internal_semClose(){
  // do stuff :)
  int fd = running->syscall_args[0];  //assegnazione file descriptor processo corrente

  //Descriptor*  DescriptorList_byFd(ListHead* l, int fd)
  SemDescriptor* fd_sem = (SemDescriptor*)SemDescriptorList_byFd(&running->sem_descriptors, fd);  //verifico che il descrittore: fd si trovi nella lista dei descrittori del semaforo

  if(!fd_sem) {  //caso in cui il descittore fd non è presente nella lista dei descrittori del semaforo
    printf("[ERROR]: Il file descriptor cercato: %d non è presente\n", fd);
    running->syscall_retvalue = DSOS_ESEM_FDES_RWRONG;
    return;
  }
  /*printf("Il file descriptor cercato: %d esiste!\n", fd);*/

  printf("Chiusura del descrittore %d associato al semaforo %d...\n", fd, fd_sem->semaphore->id);
  //ListItem* List_detach(ListHead* head, ListItem* item);
  fd_sem = (SemDescriptor*)List_detach(&running->sem_descriptors, (ListItem *)fd_sem);  //rimuovo il fd dalla lista dei descrittori del processo

  if(!fd_sem) {  //caso in cui la rimozione del descrittore, dalla lista, fallisce
    printf("[ERROR]: Rimozione del descrittore %d fallita!\n", fd);
    running->syscall_retvalue = DSOS_ELIST_DETACH;
    return;
  }

  int ret0 = SemDescriptor_free(fd_sem);  //dealloco la memoria occupata dal descriptor: fd

  //Success == 0 in PoolAllocator_releaseBlock associata a: SemDescriptor_free
  if(ret0 != 0) {  //caso in cui la deallocazione del descrittore fallisce
    printf("[ERROR]: Deallocazione della memoria associata al descrittore %d fallita!\n", fd);
    running->syscall_retvalue = DSOS_ESEM_FDES_FREE;
    return;
  }

  Semaphore* sem = fd_sem->semaphore;  //ottengo il semaforo: sem a partire dal descrittore

  printf("Rimozione del puntatore a descrittore associato al semaforo %d...\n", sem->id);
  SemDescriptorPtr* fd_semPtr = (SemDescriptorPtr*)List_detach(&sem->descriptors, (ListItem*)fd_sem->ptr);  //risalgo e rimuovo il puntatore a descrittore: fd_sem->ptr

  if(!fd_semPtr) {  //caso in cui la rimozione del puntatore a descrittore, dalla lista, fallisce
    printf("[ERROR]: Rimozione del puntatore a descrittore fallita!\n");
    running->syscall_retvalue = DSOS_ELIST_DETACH;
    return;
  }

  int ret1 = SemDescriptorPtr_free(fd_semPtr);  //dealloco la memoria occupata dal puntatore a descrittore

  //Success == 0 in PoolAllocator_releaseBlock associata a: SemDescriptorPtr_free
  if(ret1 != 0) {  //caso in cui la deallocazione del descrittore fallisce
    printf("[ERROR]: Deallocazione della memoria associata al puntatore a descrittore fallita!\n");
    running->syscall_retvalue = DSOS_ESEM_FDES_PTR_FREE;
    return;
  }

  running->last_sem_fd--;  //decremento il contatore dei descrittori attivi del processo

  if(sem->descriptors.size == 0 && sem->waiting_descriptors.size == 0) {  //controllo che non ci siano descrittori attivi e descittori in attesa nel semaforo: sem
    printf("Rimozione del semaforo con ID: %d...\n", sem->id);
    Semaphore* ret2 = (Semaphore*)List_detach(&semaphores_list, (ListItem *)sem);  //rimuovo il semaforo dalla lista dei semafori

    if(!ret2) {  //caso in cui la rimozione del semaforo, dalla lista, fallisce
      printf("[ERROR]: Rimozione del semaforo con ID: %d fallita!\n\n", sem->id);
      running->syscall_retvalue = DSOS_ELIST_DETACH;
      return;
    }

    int ret3 = Semaphore_free(sem);  //dealloco la memoria occupata dal semaforo

    //Success == 0 in PoolAllocator_releaseBlock associata a: Semaphore_free
    if(ret3 != 0) {  //caso in cui la deallocazione del descrittore fallisce
      printf("[ERROR]: Deallocazione della memoria associata al semaforo con ID: %d fallita!\n", sem->id);
      running->syscall_retvalue = DSOS_ESEM_FREE;
      return;
    }
  }
  /*else printf("Non rimuovo il semaforo con ID: %d\n", sem->id);*/  //caso in cui sono presenti descrittori attivi e/o descrittori in attesa -> NON rimuovo il semaforo

  running->syscall_retvalue = 0;  //setto il valore di ritorno a 0
}
