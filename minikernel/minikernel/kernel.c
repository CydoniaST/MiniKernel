/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */

/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Funci�n que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Funci�n que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}



/*++++++++++++++++ AÑADIDA POR NOSOTROS: INSERTAR PRIMERO "insertar_primero"++++++++++++++++++*/
static void insertar_primero(lista_BCPs *lista, BCP *proc){
	if(lista->primero != NULL)
		proc->siguiente = lista->primero;
		
	else
		proc->siguiente = NULL;
		lista->ultimo = proc;
	lista->primero = proc;

}


// ↑↑ Tras un análisis, es probable que sea innecesario 


/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){

	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
	int nivel;

	printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al m�nimo el nivel de interrupci�n mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}





/*
 * Funci�n de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
	return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;

	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no deber�a llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 * 
 *  Añadir a la rutina de interrupción la detección de si se cumple el plazo de
 *	algún proceso dormido. Si es así, debe cambiarle de estado y reajustar las
 *	listas correspondientes
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit(){

	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal(){
	char car;

	car = leer_puerto(DIR_TERMINAL);
	printk("-> TRATANDO INT. DE TERMINAL %c\n", car);

        return;
}

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){

	printk("-> TRATANDO INT. DE RELOJ\n");

		restarTiempoBloqueados();
        return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}

/*
 * Tratamiento de interrupciuones software
 */
static void int_sw(){

	printk("-> TRATANDO INT. SW\n");

	return;
}



/*Añadir a la rutina de interrupción la detección de si se cumple el plazo de
algún proceso dormido. Si es así, debe cambiarle de estado y reajustar las
listas correspondientes*/



/*
*************PREGUNTAR: ¿SE HACE DE ALGUNA DE LAS MANERAS QUE HEMOS PLANTEADO AL FINAL DE ESTA FUNCIÓN?
						¿O NO ES NECESARIO EVALUARLO, PORQUE CONTAMOS CON QUE CLOCK FUNCIONE BIEN?****







/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog){
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;

	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
	if (imagen)
	{
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;

		/* lo inserta al final de cola de listos */
		insertar_ultimo(&lista_listos, p_proc);
		error= 0;
	}
	else
		error= -1; /* fallo al crear imagen */

	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso(){
	char *prog;
	int res;

	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	res=crear_tarea(prog);
	return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);

	liberar_proceso();

        return 0; /* no deber�a llegar aqui */
}


/*
Incluir la llamada que, entre otras labores, debe poner al proceso en estado
bloqueado, reajustar las listas de BCPs correspondientes y realizar el cambio
de contexto.
*/
int sis_cambios(BCPptr actual){

	
	//Cambio de contexto
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no debería llegar aqui */
	
	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no debería llegar aqui */

		

}




/* funcion auxiliar para la llamada dormir, actualiza los tiempos de los procesos dormidos */ 
void restarTiempoBloqueados(){ 
 
	BCPptr aux = lista_bloqueados.primero; 

	/* recorro la lista y actualizo los tiempos */ 

	while(aux != NULL){ 
		BCPptr siguiente = aux->siguiente; 
		aux->tiempo_dormir--; 

		if(aux->tiempo_dormir == 0){ 
			aux->estado = LISTO; 
			eliminar_elem(&lista_bloqueados, aux); 
			insertar_ultimo(&lista_listos, aux); 
	} 
		aux = siguiente; 
	} 
} 


//proceso auxiliar para bloquear proceso y actualizar listas

void bloquear(){

	

	//poner el proceso en bloqueado
	BCPptr actual = p_proc_actual;

	 
	actual->estado = BLOQUEADO;


	//reajustar listas de BCPs
	//	1º: eliminar de listos
	eliminar_elem(&lista_listos,p_proc_actual);

	//	2º: añadir a bloqueados
	insertar_ultimo(&lista_bloqueados,p_proc_actual);


	p_proc_actual = planificador();

	

	//Como actual es un puntero del BCP debemos seguir rabajando con puntores en el cambio de contecto
	//p_proc_actual sera el proceso que se va a derpertar y actual sera el que se va a bloquear y queremos dormir
	cambio_contexto(&(actual->contexto_regs),&(p_proc_actual->contexto_regs));

}

/*	FUNCION DORMIR 		*/
int dormir(unsigned int segundos){

	//dormir llama a bloquear
	//	-bloquear lo bloquea 
	//	-actualiza tablas
	//	-cambia contexto
	//cuando pasa el tiempo   ****HAY 
	//

	//El valor pasado a la funcion se guarda en el registro 1 y utilizamos "leer_registro" para leer ese registro
	unsigned int segundos_espera = (unsigned int)leer_registro(1);
	
	
	//Se multiplican los segundos por los ticks establecidos (en este caso 100)
	actual->tiempo_dormir = segundos_espera * TICK;

	//guardamos el nivel de interrupcion
	int n_interrupcion = fijar_nivel_int(NIVEL_3);

	//poner el proceso en bloqueado
	bloquear();




	//cuando pasa el tiempo

	//vuelve 

	//Restauramos el nivel de interrupcion
	fijar_nivel_int(n_interrupcion);

}

/*        SERVICIOS MUTEX        */



/*Crear mutex:
	-Crea el mutex con el nombre y tipo especificados. 
	-Devuelve un entero que representa un descriptor para acceder al mutex. 
	-En caso de error devuelve un número negativo. 
	-Habrá que definir dos constantes, que deberían incluirse tanto en el archivo de cabecera usado por los programas 
	de usuario (“servicios.h”) como en el usado por el sistema operativo 
	(“kernel.h”), para facilitar la especificación del tipo de mutex 
	(NO_RECURSIVO y RECURSIVO) */




//funcion auxiliar a mutex: "buscar mutex por nombre", devuelve el num de mutex con ese nombre o da error
int buscar_mut_nombre(char* nombre) {
    int i = 0;
    while (i < NUM_MUT) {
        if (strcmp(lista_mut[i].nombre, nombre) == 0) {
            return i; //devolver el numero de mutex
        }
        i++;
    }
    return -1; //Error: nombre no encontrado
}




	//funcion auxiliar a mutex: busca un hueco en el array de descriptores
int buscar_hueco_descriptores() {
    int i = 0;
    while (i < NUM_MUT_PROC) {
        if (p_proc_actual->conj_descriptores[i] == -1) { //significa que es un hueco
            return i; //igual que buscando por nombre, devuelve el indice del hueco buscado
        }
        i++;
    }
    return -1; //Error: no se encuentra hueco de descriptor
}



	//funcion auxiliar a mutex: buscamos un hueco de mutex
int buscar_hueco_mutex() { 
    int i = 0; 
    while (i < NUM_MUT) { 
        if (lista_mutex[i].estado == LIBRE) { 
            return i; /* devuelve la posicion del mutex libre */ 
        } 
        i++; 
    } 
    return -1; /* devuelve -1 si no se encuentra ningún mutex libre */
}





/*Enunciado práctica: Cuando se crea un mutex, el proceso obtiene el descriptor que le permite 
	acceder al mismo. Si ya existe un mutex con ese nombre o no quedan 
	descriptores libres, se devuelve un error. En caso de que no haya error, se 
	debe comprobar si se ha alcanzado el número máximo de mutex en el 
	sistema. Si esto ocurre, se debe bloquear al proceso hasta que se elimine 
	algún mutex. La operación que crea el mutex también lo deja abierto para 
	poder ser usado, devolviendo un descriptor que permita usarlo. Se recibirá 
	como parámetro de qué tipo es el mutex (recursivo o no)*/

int crear_mutex(char *nombre, int tipo){

	nombre = (char*) leer_registro(1); 
 	tipo = (int) leer_registro(2);

	int n_interrupcion = fijar_nivel_int(NIVEL_1);


	//Comprobamos que el nombre no excede el maximo de caracteres
	if(strlen(nombre) > (MAX_NOM_MUT-1) ) {

		printk("ERROR MINIKERNEL: %s excede el max de caracteres (%d/%d) ",nombre,strlen(nombre),MAX_NOM_MUT);
		fijar_nivel_int(n_interrupcion);

		return -1; //se cierra con codigo de error

	}

	
	//Si ya existe un mutex con ese nombre se devuelve un error 
	if(buscar_mut_nombre(nombre) != -1) {
		
		printk("ERROR KERNEL. Nombre %s en uso.\n", nombre);
		fijar_nivel_int(n_interrupcion);
		return -1; // Devolvemos el error

	}
	
	int descriptor_resultado = buscar_hueco_descriptores(); //almacenamos el resultado de buscar hueco para no llamar otra vez a la funcion en el if

	if( descriptor_resultado == -1){ //Control de erorr: si no nos da un hueco sale de la funcion
 
		printk("ERROR KERNEL. No hay hueco de descriptor.\n");
		fijar_nivel_int(n_interrupcion);
		return -1; // Devolvemos el error

	} 


	int mutex_resultado = 0;
	
	// ^^^^^^^^^^^^^^^^^=204020i2        EXPLICAR QUE HACE ESTO         399999999999999999991200000000000000000000000000000000000
	while (mutex_resultado == 0) {

		int descriptor_hueco_mutex = buscar_hueco_mutex();   //Teniendo nombre y descriptor libres, buscamos un hueco en el mutex

		//Caso positivo comienza todo el proceso de creacion del mutex
		if (descriptor_hueco_mutex != -1) {

			//puntero al mutex del hueco que hemos buscado
			MUTptr mutex_actual = &lista_mut[descriptor_hueco_mutex];   //CREO, NO LO SÉ 
			mutex_actual->nombre = nombre;
			mutex_actual->estado = OCUPADO;
			num_mut_total++;


			p_proc_actual->conj_descriptores[descriptor_resultado] = descriptor_hueco_mutex;

			printf("DE puta madre %s", nombre);

			mutex_resultado = 1;
	

		} else { //caso negativo: no hay hueco para el mutex mostramos error y bloqueamos el proceso

			printk("ERROR KERNEL. Numero maximo de mutex alcanzado en el sistema.\n");
			
			//bloquear proceso HASTA QUE SE ELMINE ALGÚN MUTEX



			//¿se puede hacer sin pararlo todo con un bucle/similares?  <<<<<<<<<<<===========================================


			mutex_resultado = 0;
			num_mut_bloqueos++;
			bloquear(); //la funcion ya se encarga de actualizar listas y pasar al siguiente proceso
			


		}

	}


	fijar_nivel_int(n_interrupcion);
	return descriptor_resultado;

}



/* Devuelve un descriptor asociado a un mutex ya existente o un 
número negativo en caso de error */
int abrir_mutex(char *nombre){




}

int lock(unsigned int mutexid){





}


int unlock(unsigned int mutexid);
int cerrar_mutex(unsigned int mutexid);





/*
 *
 * Rutina de inicializaci�n invocada en arranque
 *
 */
int main(){
	/* se llega con las interrupciones prohibidas */

	instal_man_int(EXC_ARITM, exc_arit); 
	instal_man_int(EXC_MEM, exc_mem); 
	instal_man_int(INT_RELOJ, int_reloj); 
	instal_man_int(INT_TERMINAL, int_terminal); 
	instal_man_int(LLAM_SIS, tratar_llamsis); 
	instal_man_int(INT_SW, int_sw); 
	/*Añadir a la rutina de interrupción la detección de si se cumple el plazo de
	algún proceso dormido. Si es así, debe cambiarle de estado y reajustar las
	listas correspondientes*/
	instal_man_int(INT_PLAZO, int_plazo);

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */

	iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */

	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
