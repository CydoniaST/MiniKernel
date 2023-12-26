/*
*  minikernel/include/kernel.h
*
*  Minikernel. Versi�n 1.0
*
*  Fernando P�rez Costoya
*
*/

/*
*
* Fichero de cabecera que contiene definiciones usadas por kernel.c
*
*      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
*
*/

#ifndef _KERNEL_H
#define _KERNEL_H


#include "const.h"
#include "HAL.h"
#include "llamsis.h"
#include "time.h"

/*
*
* Definicion del tipo que corresponde con el BCP.
* Se va a modificar al incluir la funcionalidad pedida.
*
*/
typedef struct BCP_t *BCPptr;

typedef struct BCP_t {
	int id;				/* ident. del proceso */
	int estado;			/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
	contexto_t contexto_regs;	/* copia de regs. de UCP */
	void * pila;			/* dir. inicial de la pila */
	BCPptr siguiente;		/* puntero a otro BCP */
	void *info_mem;			/* descriptor del mapa de memoria */
	
	//supuestamente es recomendable añadir un campo "Modificar el BCP para incluir algún campo relacionado con esta llamada"
	unsigned int tiempo_dormir; // no es en SEGUNDOS, es en TICKS


	/*MUTEX*/
	int conj_descriptores[NUM_MUT_PROC];
	int n_descriptores_usados;
	

} BCP;

/*
*
* Definicion del tipo que corresponde con la cabecera de una lista
* de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
* procesos bloqueados en semaforo, etc.).
*
*/

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;

/*
*mutex*/
typedef struct MUT_t *MUTptr;

typedef struct MUT_t{

	char nombre[MAX_NOM_MUT];      //nombre que no excede la constante
	int estado;                   /* LIBRE | OCUPADO */
	int tipo;					 //Especificación del tipo RECURSIVO | NO RECURSIVO 
	int id_mut;

	lista_BCPs lista_mut_espera;  // lista de procesos que esperan al mutex
	int n_mut_espera;            // numero de procesos de mutex en espera: tamaño de la lista
	int id_poseedor_mut;		// identificador del proceso que posee al mutex
	int num_mut_bloqueos;	   // numero de mutex bloqueados
	int estado_bloqueo_mut;	  // 0 MUT_DESBLOQUEADO | 1 MUT_BLOQUEADO



} mutex;




/*
* Variable global que identifica el proceso actual
*/

BCP * p_proc_actual=NULL;

BCP * p_proc_anterior=NULL;

/*
* Variable global que representa la tabla de procesos
*/

BCP tabla_procs[MAX_PROC];

/*
* Variable global que representa la cola de procesos listos
*/
lista_BCPs lista_listos= {NULL, NULL};


//Enunciado: "Definir una lista de procesos esperando plazos"
lista_BCPs lista_bloqueados = {NULL, NULL};


//Lista de procesos esperando mutex
lista_BCPs lista_esperando_mut = {NULL, NULL};

//lista de los mutex
mutex lista_mut[NUM_MUT];
int num_mut_total; //tamaño de la lista: controlamos el numero de mutex que hay 


/*
*
* Definici�n del tipo que corresponde con una entrada en la tabla de
* llamadas al sistema.
*
*/
typedef struct{
	int (*fservicio)();
} servicio;


//función para contar el tiempo


int contar_ticks(int ticks);

/*
* Prototipos de las rutinas que realizan cada llamada al sistema
*/
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();



int obtener_id_pr();
int dormir(unsigned int segundos);

/*        SERVICIOS MUTEX        */
int crear_mutex(char *nombre, int tipo);

//Funciones aux para crear_mutex: búsqueda por nombre y búsqueda de un hueco en el array de descriptores
int buscar_mutex_nombre(char* nombre);
int buscar_hueco_mutex();
int buscar_hueco_descriptores();


int abrir_mutex(char *nombre);
int lock(unsigned int mutexid);
int unlock(unsigned int mutexid);
int cerrar_mutex(unsigned int mutexid);


/*
* Variable global que contiene las rutinas que realizan cada llamada
*/
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
					{obtener_id_pr},
					{dormir},
					{crear_mutex},
					{abrir_mutex},
					{lock},
					{unlock},
					{cerrar_mutex}
					};

#endif /* _KERNEL_H */



