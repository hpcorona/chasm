#ifndef TYPES_H_
#define TYPES_H_

#include "parser.h"
#include <map>

using namespace std;

enum TIPO_DATO {
	DATO_INTEGER,
	DATO_CHAR,
	DATO_DOUBLE
};

enum DIMENSION {
	DIMENSION_NINGUNA,
	DIMENSION_ARREGLO,
	DIMENSION_MATRIZ
};

enum TIPO_ERROR {
	NO_ERROR,
	ERROR_ETIQUETA_NO_DECLARADA,
	ERROR_TIPOS_DIFERENTES,
	ERROR_FALTAN_INDICES,
	ERROR_DEMASIADOS_INDICES
};

typedef struct {
	TIPO_DATO tipo;
	DIMENSION dimension;
	int dim1;
	int dim2;
	long direccion;
} SIMBOLO;

typedef struct {
	TIPO_ERROR error;
	TOKEN* token;
} ERROR;

enum TIPO_RUNTIME {
	RUNTIME_STRING,
	RUNTIME_VALOR,
};

#define TABLA_SIMBOLOS map<string, SIMBOLO*>

bool es_congurente_expresion(TABLA_SIMBOLOS *tabla_simbolos, PRODUCCION *expresion_tipo);
PRODUCCION* convertir_a_concatenacion_string(TABLA_SIMBOLOS *tabla_simbolos, PRODUCCION *expresion_numero);
PRODUCCION* convertir_a_expresion_numero(TABLA_SIMBOLOS *tabla_simbolos, PRODUCCION *expresion_string);
PRODUCCION* convertir_a_expresion_string(TABLA_SIMBOLOS *tabla_simbolos, PRODUCCION *expresion_numero);
TIPO_RUNTIME tipo_etiqueta(TABLA_SIMBOLOS* simbolos, PRODUCCION* etiqueta);

SIMBOLO* crear_simbolo(TIPO_DATO tipo, DIMENSION dimension, int dim1, int dim2, long direccion);
void throw_type_error(TIPO_ERROR tipo_error, TOKEN* token);
TABLA_SIMBOLOS* verificar_tipos(PRODUCCION* ch);

void verificar_produccion(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_declaracion_arreglo(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_declaracion_variable(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_etiqueta(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_asignacion(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_conversion_expresion(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion, PRODUCCION *produccion_padre, bool padre);
void verificar_expresion(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_solo_strings(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_expresion_string(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_solo_numeros(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_expresion_numero(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_etiqueta(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_expresion_string_rr(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);
void verificar_factor_condicion(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion);

#endif /*TYPES_H_*/
