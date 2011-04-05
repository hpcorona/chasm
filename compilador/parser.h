#ifndef PARSER_H_
#define PARSER_H_

#include <assert.h>
#include "tokens.h"
#include <map>

enum TIPO_PRODUCCION {
	PRODUCCION_CH,
	PRODUCCION_DATOS,
	PRODUCCION_DECLARACION_ARREGLO,
	PRODUCCION_DECLARACION_VARIABLE,
	PRODUCCION_INSTRUCCIONES,
	PRODUCCION_INSTRUCCION,
	PRODUCCION_ASIGNACION,
	PRODUCCION_LECTURA,
	PRODUCCION_ESCRITURA,
	PRODUCCION_PRINT,
	PRODUCCION_IF,
	PRODUCCION_WHILE,
	PRODUCCION_FOR,
	PRODUCCION_TIPO_DATO,
	PRODUCCION_ETIQUETA,
	PRODUCCION_EXPRESION,
	PRODUCCION_EXPRESION_STRING,
	PRODUCCION_EXPRESION_NUMERO,
	PRODUCCION_EXPRESION_STRING_RR,
	PRODUCCION_EXPRESION_NUMERO_RR,
	PRODUCCION_TERMINO,
	PRODUCCION_TERMINO_RR,
	PRODUCCION_FACTOR,
	PRODUCCION_FACTOR_CONDICION,
	PRODUCCION_CONDICION,
	PRODUCCION_CONDICION_RR,
	PRODUCCION_UNION,
	PRODUCCION_OPERADOR,
	PRODUCCION_STRING,
	PRODUCCION_LEXEMA
};

typedef struct {
	TOKEN* tokens[10000];
	int total;
	int posicion;
	
	int posicionStack[1000];
	int totalPosicion;
} CONTEXTO;

typedef struct PRODUCCION {
	TIPO_PRODUCCION tipo;
	TOKEN* token;
	PRODUCCION* hijos;
	PRODUCCION* siguiente;
};


PRODUCCION* crear_produccion(TOKEN* token);
PRODUCCION* crear_produccion(TIPO_PRODUCCION tipo);
PRODUCCION* buscar_uno_antes(PRODUCCION* actual, PRODUCCION *produccion);
void agregar_hijo(PRODUCCION* padre, PRODUCCION *produccion);
void remover_hijo(PRODUCCION* padre, PRODUCCION *produccion);
void remover_produccion(PRODUCCION *produccion);
void agregar_siguiente(PRODUCCION* produccion, PRODUCCION *nuevo);
int ancho_produccion(PRODUCCION* produccion);
int ancho_produccion_etiquetas(PRODUCCION* produccion);
int ancho_produccion_etiquetaz(PRODUCCION* produccion);
void adelantar_posicion(CONTEXTO *contexto, PRODUCCION *produccion);

void throw_token_warning(CONTEXTO* contexto, const char *warning);
void throw_token_error(TOKEN* token, const char *error);
void throw_token_error(CONTEXTO* contexto, const char *error);
PRODUCCION* consumir_token(CONTEXTO* contexto, TIPO_TOKEN tipo);
void marcar_posicion(CONTEXTO* contexto);
void resetear_posicion(CONTEXTO* contexto);
void cancelar_posicion(CONTEXTO* contexto);

PRODUCCION* produccion_ch(CONTEXTO* contexto);
PRODUCCION* produccion_datos(CONTEXTO* contexto);
PRODUCCION* produccion_declaracion_arreglo(CONTEXTO* contexto);
PRODUCCION* produccion_declaracion_variable(CONTEXTO* contexto);
PRODUCCION* produccion_instrucciones(CONTEXTO* contexto);
PRODUCCION* produccion_instruccion(CONTEXTO* contexto);
PRODUCCION* produccion_asignacion(CONTEXTO* contexto);
PRODUCCION* produccion_lectura(CONTEXTO* contexto);
PRODUCCION* produccion_escritura(CONTEXTO* contexto);
PRODUCCION* produccion_print(CONTEXTO* contexto);
PRODUCCION* produccion_if(CONTEXTO* contexto);
PRODUCCION* produccion_while(CONTEXTO* contexto);
PRODUCCION* produccion_for(CONTEXTO* contexto);
PRODUCCION* produccion_tipo_dato(CONTEXTO* contexto);
PRODUCCION* produccion_etiqueta(CONTEXTO* contexto);
PRODUCCION* produccion_expresion(CONTEXTO* contexto);
PRODUCCION* produccion_expresion_string(CONTEXTO* contexto);
PRODUCCION* produccion_string(CONTEXTO* contexto);
PRODUCCION* produccion_expresion_string_rr(CONTEXTO* contexto);
PRODUCCION* produccion_expresion_numero(CONTEXTO* contexto);
PRODUCCION* produccion_termino(CONTEXTO* contexto);
PRODUCCION* produccion_expresion_numero_rr(CONTEXTO* contexto);
PRODUCCION* produccion_termino_rr(CONTEXTO* contexto);
PRODUCCION* produccion_factor(CONTEXTO* contexto);
PRODUCCION* produccion_factor_condicion(CONTEXTO* contexto);
PRODUCCION* produccion_condicion(CONTEXTO* contexto);
PRODUCCION* produccion_condicion_rr(CONTEXTO* contexto);
PRODUCCION* produccion_union(CONTEXTO* contexto);
PRODUCCION* produccion_operador(CONTEXTO* contexto);

bool token_in_first(CONTEXTO* contexto, TIPO_TOKEN tipo);
bool tif_declaracion_arreglo(CONTEXTO* contexto);
bool tif_declaracion_variable(CONTEXTO* contexto);
bool tif_instrucciones(CONTEXTO* contexto);
bool tif_expresion(CONTEXTO* contexto);
bool tif_expresion_string(CONTEXTO* contexto);
bool tif_factor(CONTEXTO* contexto);

#define DEBUG_TABLE	map<TIPO_PRODUCCION,string>

void debug_produccion(PRODUCCION *produccion);
void debug_produccion(PRODUCCION *produccion, int nivel, DEBUG_TABLE table);

PRODUCCION* seleccionar_produccion(PRODUCCION *produccion, const char* camino);
TOKEN* seleccionar_token(PRODUCCION *produccion, const char* camino);

#endif /*PARSER_H_*/
