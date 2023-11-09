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

/* defines para el mutex  */
#define NO_RECURSIVO 0
#define RECURSIVO 1
#define LIBRE 1
#define OCUPADO 0

#define NUM_MUT //numero de mutex
#define MAX_NOM_MUT  //Limitamos el tamaño del nombre del mutex
#define NUM_MUT_PROC //Limite de descriptores por proceso

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
	

} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en sem�foro, etc.).
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

	lista_BCPs lista_mut_espera;  // lista de procesos que esperan al mutex
	int n_mut_espera;            // numero de procesos de mutex en espera: tamaño de la lista
	int id_mut;					// ident. del proceso que tiene al mutex
	int num_mut_bloqueos;	   // 




} mutex;




/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

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


/*
 *
 * Definici�n del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;


/*función para contar el tiempo
*/

int contar_ticks(int ticks);

/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();


/*
Incluir la llamada que, entre otras labores, debe poner al proceso en estado
bloqueado, reajustar las listas de BCPs correspondientes y realizar el cambio
de contexto.
*/
int sis_cambios();



/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir}};

#endif /* _KERNEL_H */


/*        SERVICIOS MUTEX        */
int crear_mutex(char *nombre, int tipo);
int abrir_mutex(char *nombre);
int lock(unsigned int mutexid);
int unlock(unsigned int mutexid);
int cerrar_mutex(unsigned int mutexid);
