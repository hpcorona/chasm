#include "types.h"

SIMBOLO* crear_simbolo(TIPO_DATO tipo, DIMENSION dimension, int dim1, int dim2, long direccion) {
	SIMBOLO *simbolo = new SIMBOLO();
	simbolo->tipo = tipo;
	simbolo->dimension = dimension;
	simbolo->dim1 = dim1;
	simbolo->dim2 = dim2;
	simbolo->direccion = direccion;
	simbolo->variable = 0;
	simbolo->alignment = 0;
	
	return simbolo;
}

void throw_type_error(TIPO_ERROR tipo_error, TOKEN* token) {
	ERROR *error = new ERROR();
	error->error = tipo_error;
	error->token = token;
	
	throw error;
}

TABLA_SIMBOLOS* verificar_tipos(PRODUCCION* ch) {
	TABLA_SIMBOLOS* tabla_simbolos = new TABLA_SIMBOLOS();

	try {	
		verificar_produccion(tabla_simbolos, ch);
	} catch (ERROR *error) {
		switch (error->error) {
		case ERROR_ETIQUETA_NO_DECLARADA:
			throw_token_error(error->token, "la etiqueta no se encuentra declarada");
			break;
		case ERROR_TIPOS_DIFERENTES:
			throw_token_error(error->token, "operacion invalida con la variable");
			break;
		case ERROR_FALTAN_INDICES:
			throw_token_error(error->token, "faltan indices en el vector");
			break;
		case ERROR_DEMASIADOS_INDICES:
			throw_token_error(error->token, "demasiados incides en la variable");
			break;
		default:
			throw_token_error(error->token, "error desconocido");
		}
	}
	
	return tabla_simbolos;
}

void verificar_hijos(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	assert(produccion != NULL);
	
	PRODUCCION* primer_hijo = produccion->hijos;
	while (primer_hijo != NULL) {
		verificar_produccion(simbolos, primer_hijo);
		
		primer_hijo = primer_hijo->siguiente;
	}
}

void verificar_produccion(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	assert(simbolos != NULL);
	assert(produccion != NULL);
	
	switch(produccion->tipo) {
	case PRODUCCION_DECLARACION_ARREGLO:
		verificar_declaracion_arreglo(simbolos, produccion);
		break;
	case PRODUCCION_DECLARACION_VARIABLE:
		verificar_declaracion_variable(simbolos, produccion);
		break;
	case PRODUCCION_ETIQUETA:
		verificar_etiqueta(simbolos, produccion);
		break;
	case PRODUCCION_ASIGNACION:
		verificar_asignacion(simbolos, produccion);
		break;
	case PRODUCCION_EXPRESION:
		verificar_expresion(simbolos, produccion);
		break;
	case PRODUCCION_EXPRESION_STRING:
		verificar_expresion_string(simbolos, produccion);
		break;
	case PRODUCCION_EXPRESION_NUMERO:
		verificar_expresion_numero(simbolos, produccion);
		break;
	case PRODUCCION_EXPRESION_STRING_RR:
		verificar_expresion_string_rr(simbolos, produccion);
		break;
	case PRODUCCION_FACTOR_CONDICION:
		verificar_factor_condicion(simbolos, produccion);
		break;	
	default:
		verificar_hijos(simbolos, produccion);
		break;
	}
}

void verificar_declaracion_arreglo(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	assert(simbolos != NULL);
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_DECLARACION_ARREGLO);
	
	PRODUCCION* etiqueta_actual;
	TIPO_DATO tipo_dato;
	DIMENSION dimension;
	int d1, d2;
	
	TOKEN* tdato = seleccionar_token(produccion, "..");
	assert(tdato != NULL);
	
	switch(tdato->tipo) {
	case TOKEN_INTEGER:
		tipo_dato = DATO_INTEGER;
		break;
	case TOKEN_DOUBLE:
		tipo_dato = DATO_DOUBLE;
		break;
	case TOKEN_CHAR:
		tipo_dato = DATO_CHAR;
		break;
	default:
		assert(false);
		break;
	}
	
	TOKEN* dim1 = seleccionar_token(produccion, ".>>");
	assert(dim1 != NULL);
	
	TOKEN* coma = seleccionar_token(produccion, ".>>>");
	if (coma != NULL && coma->tipo == TOKEN_COMA) {
		TOKEN* dim2 = seleccionar_token(produccion, ".>>>>");
		assert(dim2 != NULL);

		dimension = DIMENSION_MATRIZ;
		d1 = atoi(dim1->token.c_str());
		d2 = atoi(dim2->token.c_str());
		
		etiqueta_actual = seleccionar_produccion(produccion, ".>>>>>>");
	} else {
		dimension = DIMENSION_ARREGLO;
		d1 = atoi(dim1->token.c_str());
		d2 = 0;
		
		etiqueta_actual = seleccionar_produccion(produccion, ".>>>>");
	}
	
	assert(etiqueta_actual != NULL);
	assert(etiqueta_actual->tipo == PRODUCCION_LEXEMA);
	
	while (etiqueta_actual != NULL) {
		(*simbolos)[etiqueta_actual->token->token] = crear_simbolo(tipo_dato, dimension, d1, d2, 0);
		
		etiqueta_actual = etiqueta_actual->siguiente; // esta es una COMA (,)
		if (etiqueta_actual != NULL) {
			etiqueta_actual = etiqueta_actual->siguiente;
		}
	}
}

void verificar_declaracion_variable(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	assert(simbolos != NULL);
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_DECLARACION_VARIABLE);
	
	PRODUCCION* etiqueta_actual;
	TIPO_DATO tipo_dato;
	
	TOKEN* tdato = seleccionar_token(produccion, "..");
	assert(tdato != NULL);
	
	switch(tdato->tipo) {
	case TOKEN_INTEGER:
		tipo_dato = DATO_INTEGER;
		break;
	case TOKEN_DOUBLE:
		tipo_dato = DATO_DOUBLE;
		break;
	case TOKEN_CHAR:
		tipo_dato = DATO_CHAR;
		break;
	default:
		assert(false);
		break;
	}
	
	etiqueta_actual = seleccionar_produccion(produccion, ".>");
	
	assert(etiqueta_actual != NULL);
	assert(etiqueta_actual->tipo == PRODUCCION_LEXEMA);
	
	while (etiqueta_actual != NULL) {
		(*simbolos)[etiqueta_actual->token->token] = crear_simbolo(tipo_dato, DIMENSION_NINGUNA, 0, 0, 0);
		
		etiqueta_actual = etiqueta_actual->siguiente; // esta es una COMA (,)
		if (etiqueta_actual != NULL) {
			etiqueta_actual = etiqueta_actual->siguiente;
		}
	}
}

void verificar_asignacion(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	assert(simbolos != NULL);
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_ASIGNACION);
	
	verificar_hijos(simbolos, produccion);
	
	TIPO_RUNTIME variable = tipo_etiqueta(simbolos, seleccionar_produccion(produccion, "."));
	
	PRODUCCION* expresion = seleccionar_produccion(produccion, ".>>.");
	assert(expresion != NULL);
	
	assert(expresion->tipo == PRODUCCION_EXPRESION_STRING ||
			expresion->tipo == PRODUCCION_EXPRESION_NUMERO);
			
	if (variable == RUNTIME_VALOR && expresion->tipo == PRODUCCION_EXPRESION_STRING) {
		throw_type_error(ERROR_TIPOS_DIFERENTES, seleccionar_token(produccion, "."));
	} else if (variable == RUNTIME_STRING && expresion->tipo == PRODUCCION_EXPRESION_NUMERO) {
		PRODUCCION* expresion_padre = seleccionar_produccion(produccion, ".>>");
		
		PRODUCCION* expresion_string = convertir_a_concatenacion_string(simbolos, expresion);
		expresion_padre->hijos = expresion_string;
	}
}

void verificar_etiqueta(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	assert(simbolos != NULL);
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_ETIQUETA);
	
	// solo para asegurarnos de que se este utilizando adecuadamente
	tipo_etiqueta(simbolos, produccion);
	
	TOKEN *etiqueta = seleccionar_token(produccion, ".");
	assert(etiqueta != NULL);
	
	if ((*simbolos)[etiqueta->token] == NULL) {
		throw_type_error(ERROR_ETIQUETA_NO_DECLARADA, etiqueta);
	}
	
	verificar_hijos(simbolos, produccion);
}

void verificar_factor_condicion(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	assert(simbolos != NULL);
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_FACTOR_CONDICION);
	
	PRODUCCION* expr1 = seleccionar_produccion(produccion, ".");
	assert(expr1 != NULL);
	if (expr1->tipo != PRODUCCION_LEXEMA) {
		PRODUCCION* expr2 = seleccionar_produccion(produccion, ".>>");
		assert(expr2 != NULL);
		
		verificar_conversion_expresion(simbolos, expr1, produccion, true);
		
		PRODUCCION* operador = seleccionar_produccion(produccion, ".>");
		assert(operador != NULL);
		assert(operador->tipo == PRODUCCION_OPERADOR);
		
		verificar_conversion_expresion(simbolos, expr2, operador, false);
	}
	
	verificar_hijos(simbolos, produccion);
}

void verificar_conversion_expresion(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion, PRODUCCION *produccion_padre, bool padre) {
	assert(simbolos != NULL);
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_EXPRESION_NUMERO ||
			produccion->tipo == PRODUCCION_EXPRESION_STRING);

	if (ancho_produccion_etiquetas(produccion) == 1 &&
			es_congurente_expresion(simbolos, produccion) == false) {
		if (produccion->tipo == PRODUCCION_EXPRESION_NUMERO) {
			PRODUCCION *expresion_string = convertir_a_expresion_string(simbolos, produccion);
			if (padre == true) {
				produccion_padre->hijos = expresion_string;
			} else {
				produccion_padre->siguiente = expresion_string;
			}
		} else {
			PRODUCCION *expresion_numero = convertir_a_expresion_numero(simbolos, produccion);
			if (padre == true) {
				produccion_padre->hijos = expresion_numero;
			} else {
				produccion_padre->siguiente = expresion_numero;
			}
		}
	}
}

void verificar_expresion(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	assert(simbolos != NULL);
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_EXPRESION);
	
	PRODUCCION *expresion_hija = seleccionar_produccion(produccion, ".");
	assert(expresion_hija != NULL);
	assert(expresion_hija->tipo == PRODUCCION_EXPRESION_NUMERO ||
			expresion_hija->tipo == PRODUCCION_EXPRESION_STRING);
	
	verificar_conversion_expresion(simbolos, expresion_hija, produccion, true);
	
	verificar_hijos(simbolos, produccion);
}

void verificar_solo_strings(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	if (produccion == NULL) {
		return;
	}
	
	if (produccion->tipo == PRODUCCION_ETIQUETA) {
		if (tipo_etiqueta(simbolos, produccion) != RUNTIME_STRING) {
			throw_type_error(ERROR_TIPOS_DIFERENTES, seleccionar_token(produccion, "."));
		}
		
		return;
	} else if (produccion->tipo == PRODUCCION_EXPRESION_NUMERO) {
		verificar_solo_numeros(simbolos, produccion);
		return;
	} 
	
	verificar_solo_strings(simbolos, produccion->hijos);
	
	PRODUCCION* actual = produccion->siguiente;
	while (actual != NULL) {
		verificar_solo_strings(simbolos, actual);
		
		actual = actual->siguiente;
	}
}

void verificar_expresion_string(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	assert(simbolos != NULL);
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_EXPRESION_STRING);
	
	// primero realizar conversiones requeridas
	verificar_hijos(simbolos, produccion);
	
	verificar_solo_strings(simbolos, produccion);
}

void verificar_solo_numeros(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	if (produccion == NULL) {
		return;
	}
	
	if (produccion->tipo == PRODUCCION_ETIQUETA) {
		if (tipo_etiqueta(simbolos, produccion) != RUNTIME_VALOR) {
			throw_type_error(ERROR_TIPOS_DIFERENTES, seleccionar_token(produccion, "."));
		}
		
		return;
	} 
	
	verificar_solo_numeros(simbolos, produccion->hijos);
	
	PRODUCCION* actual = produccion->siguiente;
	while (actual != NULL) {
		verificar_solo_numeros(simbolos, actual);
		
		actual = actual->siguiente;
	}
}

void verificar_expresion_string_rr(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	assert(simbolos != NULL);
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_EXPRESION_STRING_RR);
	
	PRODUCCION *etiqueta = seleccionar_produccion(produccion, ".>.");
	assert(etiqueta != NULL);
	
	PRODUCCION *strings = seleccionar_produccion(produccion, ".>");
	assert(strings != NULL);

	if (etiqueta->tipo == PRODUCCION_ETIQUETA && strings->tipo == PRODUCCION_STRING) {
		PRODUCCION *AMP = seleccionar_produccion(produccion, ".");
		assert(AMP != NULL && AMP->tipo == PRODUCCION_LEXEMA);
		
		if (tipo_etiqueta(simbolos, etiqueta) != RUNTIME_STRING) {
			PRODUCCION *expresion_numero = crear_produccion(PRODUCCION_EXPRESION_NUMERO);
			expresion_numero->siguiente = strings->siguiente;
			strings->siguiente = NULL;
			strings->hijos = NULL;
			AMP->siguiente = expresion_numero;
			
			PRODUCCION *termino = crear_produccion(PRODUCCION_TERMINO);
			agregar_hijo(expresion_numero, termino);
			
			PRODUCCION *factor = crear_produccion(PRODUCCION_FACTOR);
			agregar_hijo(termino, factor);
			
			agregar_hijo(factor, etiqueta);
			
			remover_produccion(strings);
		}
		
		verificar_hijos(simbolos, produccion);
	} else {
		verificar_hijos(simbolos, produccion);
	}
}

void verificar_expresion_numero(TABLA_SIMBOLOS* simbolos, PRODUCCION* produccion) {
	assert(simbolos != NULL);
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_EXPRESION_NUMERO);
	
	// primero realizar conversiones requeridas
	verificar_hijos(simbolos, produccion);
	
	verificar_solo_numeros(simbolos, produccion);
}

TIPO_RUNTIME tipo_etiqueta(TABLA_SIMBOLOS* simbolos, PRODUCCION* etiqueta) {
	assert(simbolos != NULL);
	assert(etiqueta != NULL);
	assert(etiqueta->tipo == PRODUCCION_ETIQUETA);
	
	TOKEN* etiqueta_token = seleccionar_token(etiqueta, ".");
	assert(etiqueta_token != NULL);
	
	SIMBOLO* simbolo = (*simbolos)[etiqueta_token->token];
	if (simbolo == NULL) {
		throw_type_error(ERROR_ETIQUETA_NO_DECLARADA, etiqueta_token);
	}
	
	TOKEN* lcor = seleccionar_token(etiqueta, ".>");
	if (lcor != NULL && lcor->tipo == TOKEN_LCOR) {
		lcor = seleccionar_token(etiqueta, ".>>>");
		assert(lcor != NULL);
		if (lcor->tipo == TOKEN_COMA) {
			// lo estan usando como matriz v[i,j]
			switch(simbolo->dimension) {
			case DIMENSION_MATRIZ:
				return RUNTIME_VALOR;
			case DIMENSION_ARREGLO:
				throw_type_error(ERROR_DEMASIADOS_INDICES, lcor);
				break;
			case DIMENSION_NINGUNA:
				throw_type_error(ERROR_DEMASIADOS_INDICES, lcor);
				break;
			}
		} else {
			// lo estan usando como vector v[i]
			switch(simbolo->dimension) {
			case DIMENSION_MATRIZ:
				if (simbolo->tipo == DATO_CHAR) {
					return RUNTIME_STRING;
				}
				throw_type_error(ERROR_FALTAN_INDICES, lcor);
				break;
			case DIMENSION_ARREGLO:
				return RUNTIME_VALOR;
			case DIMENSION_NINGUNA:
				throw_type_error(ERROR_DEMASIADOS_INDICES, lcor);
				break;
			}
		}
	} else {
		// lo estan usando como variable v
		switch(simbolo->dimension) {
		case DIMENSION_MATRIZ:
			throw_type_error(ERROR_FALTAN_INDICES, etiqueta_token);
			break;
		case DIMENSION_ARREGLO:
			if (simbolo->tipo == DATO_CHAR) {
				return RUNTIME_STRING;
			}
			throw_type_error(ERROR_FALTAN_INDICES, etiqueta_token);
			break;
		case DIMENSION_NINGUNA:
			return RUNTIME_VALOR;
			break;
		}
	}
	
	return RUNTIME_VALOR;
}

bool es_congurente_expresion(TABLA_SIMBOLOS *tabla_simbolos, PRODUCCION *expresion_tipo) {
	assert(expresion_tipo != NULL);
	assert(expresion_tipo->tipo == PRODUCCION_EXPRESION ||
			expresion_tipo->tipo == PRODUCCION_EXPRESION_STRING ||
			expresion_tipo->tipo == PRODUCCION_EXPRESION_NUMERO);
	
	if (expresion_tipo->tipo == PRODUCCION_EXPRESION) {
		return es_congurente_expresion(tabla_simbolos, expresion_tipo->hijos);
	}
	
	if (expresion_tipo->tipo == PRODUCCION_EXPRESION_STRING &&
			ancho_produccion_etiquetas(expresion_tipo) == 1) {
		PRODUCCION *ultima = seleccionar_produccion(expresion_tipo, "..");
		
		if (ultima->tipo != PRODUCCION_ETIQUETA) {
			return true;
		} else {
			return tipo_etiqueta(tabla_simbolos, ultima) == RUNTIME_STRING;
		}
	} else if (expresion_tipo->tipo == PRODUCCION_EXPRESION_NUMERO &&
			ancho_produccion_etiquetas(expresion_tipo) == 1) {
		PRODUCCION *ultima = seleccionar_produccion(expresion_tipo, "...");
		
		if (ultima->tipo != PRODUCCION_ETIQUETA) {
			return true;
		} else {
			return tipo_etiqueta(tabla_simbolos, ultima) == RUNTIME_VALOR;
		}
	} else {
		return true;
	}
}

PRODUCCION* convertir_a_concatenacion_string(TABLA_SIMBOLOS *tabla_simbolos, PRODUCCION *expresion_numero) {
	assert(expresion_numero != NULL);
	assert(expresion_numero->tipo == PRODUCCION_EXPRESION_NUMERO);
	
	PRODUCCION* expresion_string = crear_produccion(PRODUCCION_EXPRESION_STRING);
	expresion_string->siguiente = expresion_numero->siguiente;
	expresion_numero->siguiente = NULL;
	
	PRODUCCION* strings = crear_produccion(PRODUCCION_STRING);
	agregar_hijo(expresion_string, strings);
	
	PRODUCCION* lexema = crear_produccion(PRODUCCION_LEXEMA);
	lexema->token = crear_token("\"\"", -1, -1);
	agregar_hijo(strings, lexema);
	
	PRODUCCION* expresion_string_rr = crear_produccion(PRODUCCION_EXPRESION_STRING_RR);
	agregar_hijo(expresion_string, expresion_string_rr);
	
	PRODUCCION* lexema_concat = crear_produccion(PRODUCCION_LEXEMA);
	lexema_concat->token = crear_token("&", -1, -1);
	agregar_hijo(expresion_string_rr, lexema_concat);
	
	agregar_hijo(expresion_string_rr, expresion_numero);
	
	return expresion_string;
}
 
PRODUCCION* convertir_a_expresion_numero(TABLA_SIMBOLOS *tabla_simbolos, PRODUCCION *expresion_string) {
	assert(expresion_string != NULL);
	assert(expresion_string->tipo == PRODUCCION_EXPRESION_STRING);
	assert(ancho_produccion_etiquetas(expresion_string) == 1);
	
	PRODUCCION *ultima = seleccionar_produccion(expresion_string, "..");
	assert(ultima != NULL);
	assert(ultima->tipo == PRODUCCION_ETIQUETA);
	assert(tipo_etiqueta(tabla_simbolos, ultima) == RUNTIME_VALOR);
	PRODUCCION *penultimo = seleccionar_produccion(expresion_string, ".");
	assert(penultimo != NULL);
	
	PRODUCCION *expresion_numero = crear_produccion(PRODUCCION_EXPRESION_NUMERO);
	expresion_numero->siguiente = expresion_string->siguiente;
	expresion_string->siguiente = NULL;
	
	PRODUCCION *termino = crear_produccion(PRODUCCION_TERMINO);
	agregar_hijo(expresion_numero, termino);
	
	PRODUCCION *factor = crear_produccion(PRODUCCION_FACTOR);
	agregar_hijo(termino, factor);
	
	penultimo->hijos = NULL;
	agregar_hijo(factor, ultima);

	remover_produccion(expresion_string);
	
	return expresion_numero;
}

PRODUCCION* convertir_a_expresion_string(TABLA_SIMBOLOS *tabla_simbolos, PRODUCCION *expresion_numero) {
	assert(expresion_numero != NULL);
	assert(expresion_numero->tipo == PRODUCCION_EXPRESION_NUMERO);
	assert(ancho_produccion_etiquetas(expresion_numero) == 1);
	
	PRODUCCION *ultima = seleccionar_produccion(expresion_numero, "...");
	assert(ultima != NULL);
	assert(ultima->tipo == PRODUCCION_ETIQUETA);
	assert(tipo_etiqueta(tabla_simbolos, ultima) == RUNTIME_STRING);
	PRODUCCION *penultimo = seleccionar_produccion(expresion_numero, "..");
	assert(penultimo != NULL);
	
	
	PRODUCCION *expresion_string = crear_produccion(PRODUCCION_EXPRESION_STRING);
	expresion_string->siguiente = expresion_numero->siguiente;
	expresion_numero->siguiente = NULL;
	
	PRODUCCION *strings = crear_produccion(PRODUCCION_STRING);
	agregar_hijo(expresion_string, strings);
	
	penultimo->hijos = NULL;
	agregar_hijo(strings, ultima);
	
	remover_produccion(expresion_numero);
	
	return expresion_string;
}
