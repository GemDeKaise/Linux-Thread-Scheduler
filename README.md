Anghelescu Andrei 322CA
# Thread Scheduler

## Concept

Am implementat un scheduler care foloseste un algoritm de tip preemptive round-robin. Schedulerul este implementat folosind doua liste inlantuite.

## Implementare

Codul este foarte bine comentat si detaliat. 

In fisierul "scheduler.h" sunt definite structurile de date necesare pentru implementarea schedulerului. O strucutra de Thread este definita pentru a putea retine informatii despre threaduri. O structura de Scheduler este definita pentru a putea retine informatii despre scheduler si liste inlantuite de threaduri.

In fisierul "scheduler.c" sunt implementate functiile necesare pentru a putea folosi schedulerul.

Am ales sa folosesc functiile de mutex in locul semaforului, deoarece mi s-a parut mai usor de inteles, implementat si de folosit.

## Rulare

```
make -f Makefile.checker
```

## Bibliografie
[Curs 4](https://ocw.cs.pub.ro/courses/so/cursuri/curs-04)
[Curs 8](https://ocw.cs.pub.ro/courses/so/cursuri/curs-08)
[Curs 9](https://ocw.cs.pub.ro/courses/so/cursuri/curs-09)

[Lab 8](https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-08)
#### YouTube


[Mutex - video](https://www.youtube.com/watch?v=nxMcK4AkAY0&t=299s&ab_channel=SolvingSkills)

[Round Robin - video](https://www.youtube.com/watch?v=bGtawSqmD_c&ab_channel=NaserSalah)

### Short Program Description

<img src="https://media.tenor.com/28FrkXCaR1oAAAAd/programming-multitasking.gif" width="500px"></div>
