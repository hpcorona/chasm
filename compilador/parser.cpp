#include "parser.h"

PRODUCCION* crear_produccion(TIPO_PRODUCCION tipo) {
	PRODUCCION* produccion = new PRODUCCION();
	produccion->tipo = tipo;
	produccion->token = NULL;
	produccion->hijos = NULL;
	produccion->siguiente = NULL;
	
	return produccion;
}

PRODUCCION* crear_produccion(TOKEN* token) {
	assert(token != NULL);
	
	PRODUCCION* produccion = new PRODUCCION();
	produccion->tipo = PRODUCCION_LEXEMA;
	produccion->token = token;
	produccion->hijos = NULL;
	produccion->siguiente = NULL;
	
	return produccion;
}

PRODUCCION* buscar_uno_antes(PRODUCCION* actual, PRODUCCION *produccion) {
	assert(actual != NULL);
	
	while (actual->siguiente != produccion && actual != NULL) {
		actual = actual->siguiente;
	}
	
	return actual;
}

void agregar_hijo(PRODUCCION* padre, PRODUCCION *produccion) {
	assert(padre != NULL);
	
	if (padre->hijos == NULL) {
		padre->hijos = produccion;
	} else {
		agregar_siguiente(padre->hijos, produccion);
	}
}

void remover_hijo(PRODUCCION* padre, PRODUCCION *produccion) {
	assert(padre != NULL);
	assert(padre->hijos != NULL);
	assert(produccion != NULL);
	
	if (produccion == padre->hijos) {
		padre->hijos = produccion->siguiente;
		remover_produccion(produccion);
	} else {
		PRODUCCION* ultimo = buscar_uno_antes(padre->hijos, produccion);
		
		assert(ultimo != NULL);
		
		ultimo->siguiente = produccion->siguiente;
		remover_produccion(produccion);
	}
}

void remover_produccion(PRODUCCION *produccion) {
	assert(produccion != NULL);
	
	while (produccion->hijos != NULL) {
		PRODUCCION* ultimo = buscar_uno_antes(produccion->hijos, NULL);
		
		assert(ultimo != NULL);
		
		remover_hijo(produccion, ultimo);
	}
	
	delete produccion;
}

void agregar_siguiente(PRODUCCION* produccion, PRODUCCION *nuevo) {
	assert(produccion != NULL);
	
	PRODUCCION* ultimo = buscar_uno_antes(produccion, NULL);
	
	assert(ultimo != NULL);
	
	ultimo->siguiente = nuevo;
}

bool esperar_token(TOKEN** tokens, int max, TIPO_TOKEN tipo, int posicion) {
	assert(tokens != NULL);
	
	if (posicion >= max) {
		return false;
	}
	
	assert(tokens[posicion] != NULL);
	
	return tokens[posicion]->tipo == tipo;
}

int ancho_produccion_etiquetas(PRODUCCION* produccion) {
	if (produccion == NULL) {
		return 0;
	}
	
	if (produccion->hijos != NULL && produccion->tipo != PRODUCCION_ETIQUETA) {
		return ancho_produccion_etiquetas(produccion->hijos);
	} else {
		return 1;
	}
}

int ancho_produccion_etiquetaz(PRODUCCION* produccion) {
	if (produccion == NULL) {
		return 0;
	}
	
	if (produccion->hijos != NULL && produccion->tipo != PRODUCCION_ETIQUETA) {
		return ancho_produccion_etiquetaz(produccion->hijos) + ancho_produccion_etiquetaz(produccion->siguiente);
	} else {
		return 1 + ancho_produccion_etiquetaz(produccion->siguiente);
	}
}

int ancho_produccion(PRODUCCION* produccion) {
	if (produccion == NULL) {
		return 0;
	}
	
	if (produccion->hijos != NULL) {
		return ancho_produccion(produccion->hijos) + ancho_produccion(produccion->siguiente);
	} else {
		return 1 + ancho_produccion(produccion->siguiente);
	}
}

void adelantar_posicion(CONTEXTO *contexto, PRODUCCION *produccion) {
	int ancho = ancho_produccion(produccion);
	
	contexto->posicion += ancho;
}

void throw_token_warning(CONTEXTO* contexto, const char *warning) {
	assert(contexto != NULL);
	
	if (contexto->posicion >= contexto->total) {
		cerr << "[SINTAXIS] advertencia, stack overflow" << endl;
	}
	
	assert(contexto->tokens != NULL);
	assert(contexto->tokens[contexto->posicion] != NULL);

	TOKEN* token = contexto->tokens[contexto->posicion];

	cerr << "[SINTAXIS] advertencia, " << warning << endl << 
		"           en el renglon " << token->renglon << " columna " << token->columna << endl;
}

void throw_token_error(TOKEN* token, const char *error) {
	assert(token != NULL);
	
	char *error_char = new char[200];
	
	sprintf(error_char, "[SINTAXIS] error, %s\n           en el renglon %d columna %d\n", error, token->renglon, token->columna);

	throw error_char;
}

void throw_token_error(CONTEXTO* contexto, const char *error) {
	assert(contexto != NULL);
	
	if (contexto->posicion >= contexto->total) {
		throw "[SINTAXIS] error, stack overflow";
	}
	
	assert(contexto->tokens != NULL);
	assert(contexto->tokens[contexto->posicion] != NULL);

	TOKEN* token = contexto->tokens[contexto->posicion];
	
	throw_token_error(token, error);
}

bool token_in_first(CONTEXTO* contexto, TIPO_TOKEN tipo) {
	assert(contexto != NULL);
	
	if (contexto->posicion >= contexto->total) {
		contexto->posicion = contexto->total - 1;
		
		throw_token_error(contexto, "error de sintaxis (token_in_first)");
		
		return false;
	}

	assert(contexto->tokens != NULL);
	assert(contexto->tokens[contexto->posicion] != NULL);
	
	TOKEN* token = contexto->tokens[contexto->posicion];
	
	return token->tipo == tipo;
}

PRODUCCION* consumir_token(CONTEXTO* contexto, TIPO_TOKEN tipo) {
	assert(contexto != NULL);
	
	if (token_in_first(contexto, tipo) == true) {
		PRODUCCION* produccion = crear_produccion(contexto->tokens[contexto->posicion]);
		contexto->posicion++;
		
		return produccion;
	} else {
		throw_token_error(contexto, "token invalido (consumir_token)");
		
		return NULL;
	}
}

void marcar_posicion(CONTEXTO* contexto) {
	assert(contexto != NULL);
	
	contexto->posicionStack[contexto->totalPosicion++] = contexto->posicion;
}

void resetear_posicion(CONTEXTO* contexto) {
	assert(contexto != NULL);
	
	contexto->posicion = contexto->posicionStack[--contexto->totalPosicion];
}

void cancelar_posicion(CONTEXTO* contexto) {
	assert(contexto != NULL);
	
	contexto->totalPosicion--;
}

PRODUCCION* produccion_ch(CONTEXTO* contexto) {
	assert(contexto != NULL);
	
	// <ch> ::= <datos> <START> <instrucciones> <END>
	PRODUCCION *ch, *datos, *START, *instrucciones, *END;

	try {
		ch = crear_produccion(PRODUCCION_CH);
		
		datos = produccion_datos(contexto);
		ch->hijos = datos;
		
		START = consumir_token(contexto, TOKEN_START);
		datos->siguiente = START;
		
		instrucciones = produccion_instrucciones(contexto);
		START->siguiente = instrucciones;
		
		END = consumir_token(contexto, TOKEN_END);
		instrucciones->siguiente = END;
	} catch (char *error) {
		if (ch != NULL) {
			remover_produccion(ch);
		}
		
		throw error;
	}
	
	return ch;
}

PRODUCCION* produccion_datos(CONTEXTO* contexto) {
	assert(contexto != NULL);
	
	// <datos> ::= { <declaracion arreglo> | <declaracion variable> }
	PRODUCCION *datos, *declaracion_arreglo, *declaracion_variable;
	
	try {
		datos = crear_produccion(PRODUCCION_DATOS);
		
		while (tif_declaracion_arreglo(contexto) || tif_declaracion_variable(contexto)) {
			bool encontroArreglo = false;
			
			marcar_posicion(contexto);
			try {
				declaracion_arreglo = produccion_declaracion_arreglo(contexto);
				agregar_hijo(datos, declaracion_arreglo);
				
				encontroArreglo = true;
				cancelar_posicion(contexto);
			} catch (char *error) {
				// ignorar, quizas concuerde con una variable :(
				resetear_posicion(contexto);
				
				delete error;
			}
			
			if (encontroArreglo == false) {
				try {
					declaracion_variable = produccion_declaracion_variable(contexto);
					agregar_hijo(datos, declaracion_variable);
				} catch (char *error) {
					throw error;
				}
			}
		}
	} catch (char *error) {
		if (datos != NULL) {
			remover_produccion(datos);
		}
		
		throw error;
	}
	
	return datos;
}

PRODUCCION* produccion_declaracion_arreglo(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <declaracion arreglo> ::= <tipo dato> "[" <entero> [ "," <entero> ] "]"
	// 		<ETIQUETA> { "," <ETIQUETA> }

	PRODUCCION *declaracion_arreglo, *tipo_dato, *LCOR, *entero_positivo, *COMA, *RCOR,
			*ETIQUETA;
	
	try {
		declaracion_arreglo = crear_produccion(PRODUCCION_DECLARACION_ARREGLO);
		
		tipo_dato = produccion_tipo_dato(contexto);
		agregar_hijo(declaracion_arreglo, tipo_dato);
		
		LCOR = consumir_token(contexto, TOKEN_LCOR);
		agregar_hijo(declaracion_arreglo, LCOR);
		
		entero_positivo = consumir_token(contexto, TOKEN_ENTERO);
		agregar_hijo(declaracion_arreglo, entero_positivo);
		
		if (token_in_first(contexto, TOKEN_COMA) == true) {
			COMA = consumir_token(contexto, TOKEN_COMA);
			agregar_hijo(declaracion_arreglo, COMA);
			
			entero_positivo = consumir_token(contexto, TOKEN_ENTERO);
			agregar_hijo(declaracion_arreglo, entero_positivo);
		}
		
		RCOR = consumir_token(contexto, TOKEN_RCOR);
		agregar_hijo(declaracion_arreglo, RCOR);
		
		ETIQUETA = consumir_token(contexto, TOKEN_ETIQUETA);
		agregar_hijo(declaracion_arreglo, ETIQUETA);
		
		while (token_in_first(contexto, TOKEN_COMA) == true) {
			COMA = consumir_token(contexto, TOKEN_COMA);
			agregar_hijo(declaracion_arreglo, COMA);
			
			ETIQUETA = consumir_token(contexto, TOKEN_ETIQUETA);
			agregar_hijo(declaracion_arreglo, ETIQUETA);
		}
	} catch (char *error) {
		if (declaracion_arreglo != NULL) {
			remover_produccion(declaracion_arreglo);
		}
		
		throw error;
	}
			
	return declaracion_arreglo;
}

PRODUCCION* produccion_declaracion_variable(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <declaracion variable> ::= <tipo dato> <ETIQUETA> { "," <ETIQUETA> }
	PRODUCCION *declaracion_variable, *tipo_dato, *ETIQUETA, *COMA;
	
	try {
		declaracion_variable = crear_produccion(PRODUCCION_DECLARACION_VARIABLE);
		
		tipo_dato = produccion_tipo_dato(contexto);
		agregar_hijo(declaracion_variable, tipo_dato);
				
		ETIQUETA = consumir_token(contexto, TOKEN_ETIQUETA);
		agregar_hijo(declaracion_variable, ETIQUETA);
		
		while (token_in_first(contexto, TOKEN_COMA) == true) {
			COMA = consumir_token(contexto, TOKEN_COMA);
			agregar_hijo(declaracion_variable, COMA);
			
			ETIQUETA = consumir_token(contexto, TOKEN_ETIQUETA);
			agregar_hijo(declaracion_variable, ETIQUETA);
		}
	} catch (char *error) {
		if (declaracion_variable != NULL) {
			remover_produccion(declaracion_variable);
		}
		
		throw error;
	}
			
	return declaracion_variable;
}

PRODUCCION* produccion_instrucciones(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <instrucciones> ::= { <instruccion> }
	PRODUCCION *instrucciones, *instruccion;
	
	try {
		instrucciones = crear_produccion(PRODUCCION_INSTRUCCIONES);
		
		while (tif_instrucciones(contexto) == true) {
			instruccion = produccion_instruccion(contexto);
			agregar_hijo(instrucciones, instruccion);
		}
	} catch (char *error) {
		if (instrucciones != NULL) {
			remover_produccion(instrucciones);
		}
		
		throw error;
	}
	
	return instrucciones;
}

PRODUCCION* produccion_instruccion(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <instruccion> ::= <asignacion> | <lectura> | <escritura> | <if> |
	// 		<while> | <for> | <STOP>
	PRODUCCION *instruccion, *asignacion, *lectura, *escritura, *ifs,
			*whiles, *fors, *STOP;
	
	try {
		instruccion = crear_produccion(PRODUCCION_INSTRUCCION);
		
		if (token_in_first(contexto, TOKEN_ETIQUETA) == true) {
			asignacion = produccion_asignacion(contexto);
			agregar_hijo(instruccion, asignacion);
		} else if (token_in_first(contexto, TOKEN_READ) == true) {
			lectura = produccion_lectura(contexto);
			agregar_hijo(instruccion, lectura);
		} else if (token_in_first(contexto, TOKEN_PRINT) == true ||
				token_in_first(contexto, TOKEN_PRINTLN) == true) {
			escritura = produccion_escritura(contexto);
			agregar_hijo(instruccion, escritura);
		} else if (token_in_first(contexto, TOKEN_IF) == true) {
			ifs = produccion_if(contexto);
			agregar_hijo(instruccion, ifs);
		} else if (token_in_first(contexto, TOKEN_WHILE) == true) {
			whiles = produccion_while(contexto);
			agregar_hijo(instruccion, whiles);
		} else if (token_in_first(contexto, TOKEN_FOR) == true) {
			fors = produccion_for(contexto);
			agregar_hijo(instruccion, fors);
		} else if (token_in_first(contexto, TOKEN_STOP) == true) {
			STOP = consumir_token(contexto, TOKEN_STOP);
			agregar_hijo(instruccion, STOP);
		} else {
			throw_token_error(contexto, "se esperaba: variable, read, print, println, if, while, for, stop, end (produccion_instruccion)");
		}
	} catch (char *error) {
		if (instruccion != NULL) {
			remover_produccion(instruccion);
		}
		
		throw error;
	}
	
	return instruccion;
}

PRODUCCION* produccion_asignacion(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <asignacion> ::= <etiqueta> "=" <expresion>
	PRODUCCION *asignacion, *etiqueta, *ASIGNA, *expresion;
	
	try {
		asignacion = crear_produccion(PRODUCCION_ASIGNACION);
		
		etiqueta = produccion_etiqueta(contexto);
		agregar_hijo(asignacion, etiqueta);
		
		ASIGNA = consumir_token(contexto, TOKEN_ASIGNA);
		agregar_hijo(asignacion, ASIGNA);
		
		expresion = produccion_expresion(contexto);
		agregar_hijo(asignacion, expresion);
	} catch (char *error) {
		if (asignacion != NULL) {
			remover_produccion(asignacion);
		}
		
		throw error;
	}
	
	return asignacion;
}

PRODUCCION* produccion_lectura(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <lectura> ::= <READ> "(" <etiqueta> ")"
	PRODUCCION *lectura, *READ, *LPAR, *etiqueta, *RPAR;
	
	try {
		lectura = crear_produccion(PRODUCCION_LECTURA);
		
		READ = consumir_token(contexto, TOKEN_READ);
		agregar_hijo(lectura, READ);
		
		LPAR = consumir_token(contexto, TOKEN_LPAR);
		agregar_hijo(lectura, LPAR);
		
		etiqueta = produccion_etiqueta(contexto);
		agregar_hijo(lectura, etiqueta);
		
		RPAR = consumir_token(contexto, TOKEN_RPAR);
		agregar_hijo(lectura, RPAR);
	} catch (char *error) {
		if (lectura != NULL) {
			remover_produccion(lectura);
		}
		
		throw error;
	}
	
	return lectura;
}

PRODUCCION* produccion_escritura(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <escritura> ::= <print> "(" <expresion> ")"
	PRODUCCION *escritura, *print, *LPAR, *expresion, *RPAR;
	
	try {
		escritura = crear_produccion(PRODUCCION_ESCRITURA);
		
		print = produccion_print(contexto);
		agregar_hijo(escritura, print);
		
		LPAR = consumir_token(contexto, TOKEN_LPAR);
		agregar_hijo(escritura, LPAR);
		
		expresion = produccion_expresion(contexto);
		agregar_hijo(escritura, expresion);
		
		RPAR = consumir_token(contexto, TOKEN_RPAR);
		agregar_hijo(escritura, RPAR);
	} catch (char *error) {
		if (escritura != NULL) {
			remover_produccion(escritura);
		}
		
		throw error;
	}
	
	return escritura;
}

PRODUCCION* produccion_print(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <print> ::= <PRINT> | <PRINTLN>
	PRODUCCION *print, *PRINT, *PRINTLN;
	
	try {
		print = crear_produccion(PRODUCCION_PRINT);
		
		if (token_in_first(contexto, TOKEN_PRINT) == true) {
			PRINT = consumir_token(contexto, TOKEN_PRINT);
			agregar_hijo(print, PRINT);
		} else if (token_in_first(contexto, TOKEN_PRINTLN) == true) {
			PRINTLN = consumir_token(contexto, TOKEN_PRINTLN);
			agregar_hijo(print, PRINTLN);
		} else {
			throw_token_error(contexto, "se esperaba: print, println (produccion_print)");
		}
	} catch (char *error) {
		if (print != NULL) {
			remover_produccion(print);
		}
		
		throw error;
	}
	
	return print;
}

PRODUCCION* produccion_if(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <if> ::= <IF> "(" <condicion> ")" <instrucciones>
	// 		[ <ELSE> <instrucciones> ] <ENDIF>
	PRODUCCION *ifs, *IF, *LPAR, *condicion, *RPAR, *instrucciones,
			*ELSE, *ENDIF;
	
	try {
		ifs = crear_produccion(PRODUCCION_IF);
		
		IF = consumir_token(contexto, TOKEN_IF);
		agregar_hijo(ifs, IF);
		
		LPAR = consumir_token(contexto, TOKEN_LPAR);
		agregar_hijo(ifs, LPAR);
		
		condicion = produccion_condicion(contexto);
		agregar_hijo(ifs, condicion);
		
		RPAR = consumir_token(contexto, TOKEN_RPAR);
		agregar_hijo(ifs, RPAR);
		
		instrucciones = produccion_instrucciones(contexto);
		agregar_hijo(ifs, instrucciones);
		
		if (token_in_first(contexto, TOKEN_ELSE) == true) {
			ELSE = consumir_token(contexto, TOKEN_ELSE);
			agregar_hijo(ifs, ELSE);

			instrucciones = produccion_instrucciones(contexto);
			agregar_hijo(ifs, instrucciones);
		}
		
		ENDIF = consumir_token(contexto, TOKEN_ENDIF);
		agregar_hijo(ifs, ENDIF);
	} catch (char *error) {
		if (ifs != NULL) {
			remover_produccion(ifs);
		}
		
		throw error;
	}
	
	return ifs;
}

PRODUCCION* produccion_while(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <while> ::= <WHILE> "(" <condicion> ")" <instrucciones> <ENDWHILE>
	PRODUCCION *whiles, *WHILE, *LPAR, *condicion, *RPAR, *instrucciones, *ENDWHILE;
	
	try {
		whiles = crear_produccion(PRODUCCION_WHILE);
		
		WHILE = consumir_token(contexto, TOKEN_WHILE);
		agregar_hijo(whiles, WHILE);
		
		LPAR = consumir_token(contexto, TOKEN_LPAR);
		agregar_hijo(whiles, LPAR);
		
		condicion = produccion_condicion(contexto);
		agregar_hijo(whiles, condicion);
		
		RPAR = consumir_token(contexto, TOKEN_RPAR);
		agregar_hijo(whiles, RPAR);
		
		instrucciones = produccion_instrucciones(contexto);
		agregar_hijo(whiles, instrucciones);
		
		ENDWHILE = consumir_token(contexto, TOKEN_ENDWHILE);
		agregar_hijo(whiles, ENDWHILE);
	} catch (char *error) {
		if (whiles != NULL) {
			remover_produccion(whiles);
		}
		
		throw error;
	}
	
	return whiles;
}

PRODUCCION* produccion_for(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <for> ::= <FOR> "(" <asignacion> "," <condicion> "," <asignacion> ")"
	// 		<instrucciones> <ENDFOR>
	PRODUCCION *fors, *FOR, *LPAR, *asignacion, *COMA, *condicion, *incremento, *RPAR,
			*instrucciones, *ENDFOR;
			
	try {
		fors = crear_produccion(PRODUCCION_FOR);
		
		FOR = consumir_token(contexto, TOKEN_FOR);
		agregar_hijo(fors, FOR);
		
		LPAR = consumir_token(contexto, TOKEN_LPAR);
		agregar_hijo(fors, LPAR);
		
		asignacion = produccion_asignacion(contexto);
		agregar_hijo(fors, asignacion);
		
		COMA = consumir_token(contexto, TOKEN_COMA);
		agregar_hijo(fors, COMA);
		
		condicion = produccion_condicion(contexto);
		agregar_hijo(fors, condicion);
		
		COMA = consumir_token(contexto, TOKEN_COMA);
		agregar_hijo(fors, COMA);
		
		incremento = produccion_asignacion(contexto);
		agregar_hijo(fors, incremento);
		
		RPAR = consumir_token(contexto, TOKEN_RPAR);
		agregar_hijo(fors, RPAR);
		
		instrucciones = produccion_instrucciones(contexto);
		agregar_hijo(fors, instrucciones);
		
		ENDFOR = consumir_token(contexto, TOKEN_ENDFOR);
		agregar_hijo(fors, ENDFOR);
	} catch (char *error) {
		if (fors != NULL) {
			remover_produccion(fors);
		}
		
		throw error;
	}
	
	return fors;
}

PRODUCCION* produccion_tipo_dato(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <tipo dato> ::= <INTEGER> | <DOUBLE> | <CHAR>
	PRODUCCION *tipo_dato, *INTEGER, *DOUBLE, *CHAR;
	
	try {
		tipo_dato = crear_produccion(PRODUCCION_TIPO_DATO);
		
		if (token_in_first(contexto, TOKEN_INTEGER) == true) {
			INTEGER = consumir_token(contexto, TOKEN_INTEGER);
			agregar_hijo(tipo_dato, INTEGER);
		} else if (token_in_first(contexto, TOKEN_CHAR) == true) {
			CHAR = consumir_token(contexto, TOKEN_CHAR);
			agregar_hijo(tipo_dato, CHAR);
		} else if (token_in_first(contexto, TOKEN_DOUBLE) == true) {
			DOUBLE = consumir_token(contexto, TOKEN_DOUBLE);
			agregar_hijo(tipo_dato, DOUBLE);
		} else {
			throw_token_error(contexto, "se esperaba: integer, char, double (tipo_dato)");
		}
	} catch (char *error) {
		if (tipo_dato != NULL) {
			remover_produccion(tipo_dato);
		}
		
		throw error;
	}
	
	return tipo_dato;
}

PRODUCCION* produccion_etiqueta(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <etiqueta> ::= <ETIQUETA> [ "[" <expresion numero> [ "," <expresion numero> ] "]" ]
	PRODUCCION *etiqueta, *ETIQUETA, *LCOR, *expresion_numero, *COMA, *RCOR;
	
	try {
		etiqueta = crear_produccion(PRODUCCION_ETIQUETA);
		
		ETIQUETA = consumir_token(contexto, TOKEN_ETIQUETA);
		agregar_hijo(etiqueta, ETIQUETA);
		
		if (token_in_first(contexto, TOKEN_LCOR) == true) {
			LCOR = consumir_token(contexto, TOKEN_LCOR);
			agregar_hijo(etiqueta, LCOR);
			
			expresion_numero = produccion_expresion_numero(contexto);
			agregar_hijo(etiqueta, expresion_numero);
			
			if (token_in_first(contexto, TOKEN_COMA) == true) {
				COMA = consumir_token(contexto, TOKEN_COMA);
				agregar_hijo(etiqueta, COMA);
				
				expresion_numero = produccion_expresion_numero(contexto);
				agregar_hijo(etiqueta, expresion_numero);
			}
			
			RCOR = consumir_token(contexto, TOKEN_RCOR);
			agregar_hijo(etiqueta, RCOR);
		}
	} catch (char *error) {
		if (etiqueta != NULL) {
			remover_produccion(etiqueta);
		}
		
		throw error;
	}
	
	return etiqueta;
}

PRODUCCION* produccion_expresion(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <expresion> ::= <expresion string> | <expresion numero>
	PRODUCCION *expresion, *expresion_string, *expresion_numero;
	
	try {
		expresion = crear_produccion(PRODUCCION_EXPRESION);
		
		bool match = false;
		
		if (tif_expresion(contexto) == true 
				|| tif_factor(contexto) == true) {
			marcar_posicion(contexto);
			try {
				expresion_numero = produccion_expresion_numero(contexto);
				agregar_hijo(expresion, expresion_numero);
				
				cancelar_posicion(contexto);
				match = true;
			} catch (char *error) {
				resetear_posicion(contexto);
				
				delete error;
			}
		}
		
		if (match == false && tif_expresion_string(contexto) == true) {
			try {
				expresion_string = produccion_expresion_string(contexto);
				agregar_hijo(expresion, expresion_string);
			} catch (char *error) {
				
				delete error;
				
				throw_token_error(contexto, "se esperaba una expresion (expresion)");
			}
		}
	} catch (char *error) {
		if (expresion != NULL) {
			remover_produccion(expresion);
		}
		
		throw error;
	}
	
	return expresion;
}

PRODUCCION* produccion_expresion_string(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <expresion string> ::= <string> [ <expresion string rr> ]
	PRODUCCION *expresion_string, *strings, *expresion_string_rr;
	
	try {
		expresion_string = crear_produccion(PRODUCCION_EXPRESION_STRING);
		
		strings = produccion_string(contexto);
		agregar_hijo(expresion_string, strings);
		
		marcar_posicion(contexto);
		try {
			expresion_string_rr = produccion_expresion_string_rr(contexto);
			agregar_hijo(expresion_string, expresion_string_rr);
			
			cancelar_posicion(contexto);
		} catch (char *error) {
			resetear_posicion(contexto);
			delete error;
		}
	} catch (char *error) {
		if (expresion_string != NULL) {
			remover_produccion(expresion_string);
		}
		
		throw error;
	}
	
	return expresion_string;
}

PRODUCCION* produccion_expresion_string_rr(CONTEXTO *contexto) {
	assert(contexto != NULL);
	
	// <expresion string rr> ::= "&" <string> [ <expresion string rr> ] |
	//  	"&" <expresion numero> [ <expresion string rr> ]
	PRODUCCION *expresion_string_rr, *AMP, *strings, *expresion_string_rrs,
				*expresion_numero;
	expresion_string_rr = AMP = strings = expresion_string_rrs =
		expresion_numero = NULL;
		
	try {
		expresion_string_rr = crear_produccion(PRODUCCION_EXPRESION_STRING_RR);
		
		AMP = consumir_token(contexto, TOKEN_CONCAT);
		agregar_hijo(expresion_string_rr, AMP);
		
		marcar_posicion(contexto);
		try {
			expresion_numero = produccion_expresion_numero(contexto);
			
			resetear_posicion(contexto);
		} catch (char *error) {
			delete error;
			
			expresion_numero = NULL;
			
			resetear_posicion(contexto);
		}
		
		marcar_posicion(contexto);
		try {
			strings = produccion_string(contexto);
			
			resetear_posicion(contexto);
		} catch (char *error) {
			delete error;
			
			strings = NULL;
			
			resetear_posicion(contexto);
		}
		
		if (expresion_numero == NULL && strings == NULL) {
			throw_token_error(contexto, "expresion invalida (expresion_string_rr)");
		} else if (ancho_produccion(expresion_numero) > ancho_produccion(strings)) {
			agregar_hijo(expresion_string_rr, expresion_numero);
			adelantar_posicion(contexto, expresion_numero);

			if (strings != NULL) {			
				remover_produccion(strings);
			}
		} else {
			agregar_hijo(expresion_string_rr, strings);
			adelantar_posicion(contexto, strings);
			
			if (expresion_numero != NULL) {
				remover_produccion(expresion_numero);
			}
		}
		
		marcar_posicion(contexto);
		try {
			expresion_string_rrs = produccion_expresion_string_rr(contexto);
			agregar_hijo(expresion_string_rr, expresion_string_rrs);
			
			cancelar_posicion(contexto);
		} catch (char *err) {
			delete err;
			
			resetear_posicion(contexto);
		}
	} catch (char *error) {
		if (expresion_string_rr != NULL) {
			remover_produccion(expresion_string_rr);
		}
		
		throw error;
	}
	
	return expresion_string_rr;
}

PRODUCCION* produccion_string(CONTEXTO* contexto) {
	assert(contexto != NULL);
	
	// <string> ::= <CADENA STRING> | <etiqueta>
	PRODUCCION *strings, *CADENA_STRING, *etiqueta;
	
	try {
		strings = crear_produccion(PRODUCCION_STRING);
		
		bool encontro = false;
		marcar_posicion(contexto);
		try {
			etiqueta = produccion_etiqueta(contexto);
			agregar_hijo(strings, etiqueta);
			
			encontro = true;
			cancelar_posicion(contexto);
		} catch (char *error) {
			resetear_posicion(contexto);
			
			delete error;
		}
		
		if (encontro == false) {
			CADENA_STRING = consumir_token(contexto, TOKEN_STRING);
			agregar_hijo(strings, CADENA_STRING);
		}
	} catch (char *error) {
		if (strings != NULL) {
			remover_produccion(strings);
		}
		
		throw error;
	}
	
	return strings;
}

PRODUCCION* produccion_expresion_numero(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <expresion numero> ::= <termino> [ <expresion numero rr> ]
	PRODUCCION *expresion_numero, *termino, *expresion_numero_rr;
	
	try {
		expresion_numero = crear_produccion(PRODUCCION_EXPRESION_NUMERO);
		
		termino = produccion_termino(contexto);
		agregar_hijo(expresion_numero, termino);
		
		marcar_posicion(contexto);
		try {
			expresion_numero_rr = produccion_expresion_numero_rr(contexto);
			agregar_hijo(expresion_numero, expresion_numero_rr);
			
			cancelar_posicion(contexto);
		} catch (char *error) {
			delete error;
			
			resetear_posicion(contexto);
		}
	} catch (char *error) {
		if (expresion_numero != NULL) {
			remover_produccion(expresion_numero);
		}
		
		throw error;
	}
	
	return expresion_numero;
}

PRODUCCION* produccion_expresion_numero_rr(CONTEXTO *contexto) {
	assert(contexto != NULL);
	
	// <expresion numero rr> ::= "+" <termino> [ <expresion numero rr> ] |
	//  	"-" <termino> [ <expresion numero rr> ]
	PRODUCCION *expresion_numero_rr, *ADD, *REST, *termino, *expresion_numero_rrs;
	
	try {
		expresion_numero_rr = crear_produccion(PRODUCCION_EXPRESION_NUMERO_RR);
		
		if (token_in_first(contexto, TOKEN_ADD) == true) {
			ADD = consumir_token(contexto, TOKEN_ADD);
			agregar_hijo(expresion_numero_rr, ADD);
		} else {
			REST = consumir_token(contexto, TOKEN_REST);
			agregar_hijo(expresion_numero_rr, REST);
		}
		
		termino = produccion_termino(contexto);
		agregar_hijo(expresion_numero_rr, termino);
		
		marcar_posicion(contexto);
		try {
			expresion_numero_rrs = produccion_expresion_numero_rr(contexto);
			agregar_hijo(expresion_numero_rr, expresion_numero_rrs);
			
			cancelar_posicion(contexto);
		} catch (char *error) {
			delete error;
			
			resetear_posicion(contexto);
		}
	} catch (char *error) {
		if (expresion_numero_rr != NULL) {
			remover_produccion(expresion_numero_rr);
		}
		
		throw error;
	}
	
	return expresion_numero_rr;
}

PRODUCCION* produccion_termino(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <termino> ::= <factor> [ <termino rr> ]
	PRODUCCION *termino, *factor, *termino_rr;
	
	try {
		termino = crear_produccion(PRODUCCION_TERMINO);
		
		factor = produccion_factor(contexto);
		agregar_hijo(termino, factor);

		marcar_posicion(contexto);		
		try {
			termino_rr = produccion_termino_rr(contexto);
			agregar_hijo(termino, termino_rr);
			
			cancelar_posicion(contexto);
		} catch (char *error) {
			delete error;
			
			resetear_posicion(contexto);
		}
	} catch (char *error) {
		if (termino != NULL) {
			remover_produccion(termino);
		}
		
		throw error;
	}
	
	return termino;
}

PRODUCCION* produccion_termino_rr(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <termino rr> ::= "*" <factor> [ <termino rr> ] |
	//  	"/" <factor> [ <termino rr> ] |
	//  	"%" <factor> [ <termino rr> ]
	PRODUCCION *termino_rr, *MUL, *DIV, *MOD, *factor, *termino_rrs;
	
	try {
		termino_rr = crear_produccion(PRODUCCION_TERMINO_RR);
		
		if (token_in_first(contexto, TOKEN_MUL) == true) {
			MUL = consumir_token(contexto, TOKEN_MUL);
			agregar_hijo(termino_rr, MUL);
		} else if (token_in_first(contexto, TOKEN_DIV) == true) {
			DIV = consumir_token(contexto, TOKEN_DIV);
			agregar_hijo(termino_rr, DIV);
		} else {
			MOD = consumir_token(contexto, TOKEN_MOD);
			agregar_hijo(termino_rr, MOD);
		}
		
		factor = produccion_factor(contexto);
		agregar_hijo(termino_rr, factor);

		marcar_posicion(contexto);		
		try {
			termino_rrs = produccion_termino_rr(contexto);
			agregar_hijo(termino_rr, termino_rrs);
			
			cancelar_posicion(contexto);
		} catch (char *error) {
			delete error;
			
			resetear_posicion(contexto);
		}
	} catch (char *error) {
		if (termino_rr != NULL) {
			remover_produccion(termino_rr);
		}
		
		throw error;
	}
	
	return termino_rr;
}

PRODUCCION* produccion_factor(CONTEXTO* contexto) {
	assert(contexto != NULL);

	//<factor> ::= <numero> | <etiqueta> |
	//  	"-" <numero> |
	//  	"-" <etiqueta> |
	//  	"(" <expresion numero> ")"
	PRODUCCION *factor, *NUMERO, *ENTERO, *etiqueta, *LPAR, *expresion_numero, *RPAR, *REST;
	
	try {
		factor = crear_produccion(PRODUCCION_FACTOR);
		
		if (token_in_first(contexto, TOKEN_NUMERO) == true) {
			NUMERO = consumir_token(contexto, TOKEN_NUMERO);
			agregar_hijo(factor, NUMERO);
		} else if (token_in_first(contexto, TOKEN_ENTERO) == true) {
			ENTERO = consumir_token(contexto, TOKEN_ENTERO);
			agregar_hijo(factor, ENTERO);
		} else if (token_in_first(contexto, TOKEN_ETIQUETA) == true) {
			etiqueta = produccion_etiqueta(contexto);
			agregar_hijo(factor, etiqueta);
		} else if (token_in_first(contexto, TOKEN_LPAR) == true) {
			LPAR = consumir_token(contexto, TOKEN_LPAR);
			agregar_hijo(factor, LPAR);
			
			expresion_numero = produccion_expresion_numero(contexto);
			agregar_hijo(factor, expresion_numero);
			
			RPAR = consumir_token(contexto, TOKEN_RPAR);
			agregar_hijo(factor, RPAR);
		} else if (token_in_first(contexto, TOKEN_REST) == true) {
			marcar_posicion(contexto);
			REST = consumir_token(contexto, TOKEN_REST);
			agregar_hijo(factor, REST);
			
			bool match = true;
			
			if (token_in_first(contexto, TOKEN_NUMERO) == true) {
				NUMERO = consumir_token(contexto, TOKEN_NUMERO);
				agregar_hijo(factor, NUMERO);
			} else if (token_in_first(contexto, TOKEN_ENTERO) == true) {
				ENTERO = consumir_token(contexto, TOKEN_ENTERO);
				agregar_hijo(factor, ENTERO);
			} else if (token_in_first(contexto, TOKEN_ETIQUETA) == true) {
				etiqueta = produccion_etiqueta(contexto);
				agregar_hijo(factor, etiqueta);
			} else {
				match = false;
				resetear_posicion(contexto);
			}
			
			if (match == true) {
				cancelar_posicion(contexto);
			} else {
				throw_token_error(contexto, "expresion incompleta");
			}
		} else {
			throw_token_error(contexto, "se esperaba: numero, variable, (, -numero, -variable");
		}
	} catch (char *error) {
		if (factor != NULL) {
			remover_produccion(factor);
		}
		
		throw error;
	}
	
	return factor;
}

PRODUCCION* produccion_factor_condicion(CONTEXTO* contexto) {
	assert(contexto != NULL);
	
	// <factor condicion> ::= <expresion string> <operador> <expresion string> |
	//  	<expresion numero> <operador> <expresion numero> |
	//  	"(" <condicion> ")"
	PRODUCCION *factor_condicion, *expresion_string, *operador1, *operador2, *expresion_string2,
				*expresion_numero, *expresion_numero2, *LPAR, *condicion, *RPAR;
	
	factor_condicion = expresion_string = operador1 = operador2 = expresion_string2 =
		expresion_numero = expresion_numero2 = LPAR = condicion = RPAR = NULL;
				
	try {
		factor_condicion = crear_produccion(PRODUCCION_FACTOR_CONDICION);
		
		bool match = false;
		
		marcar_posicion(contexto);
		try {
			expresion_string = produccion_expresion_string(contexto);
			operador1 = produccion_operador(contexto);
			expresion_string2 = produccion_expresion_string(contexto);
			
			resetear_posicion(contexto);
		} catch (char *error) {
			delete error;
			
			if (expresion_string != NULL) {
				remover_produccion(expresion_string);
				expresion_string = NULL;
			}
			if (operador1 != NULL) {
				remover_produccion(operador1);
				operador1 = NULL;
			}
			if (expresion_string2 != NULL) {
				remover_produccion(expresion_string2);
				expresion_string2 = NULL;
			}
			
			resetear_posicion(contexto);
		}
		
		marcar_posicion(contexto);
		try {
			expresion_numero = produccion_expresion_numero(contexto);
			operador2 = produccion_operador(contexto);
			expresion_numero2 = produccion_expresion_numero(contexto);
			
			resetear_posicion(contexto);
		} catch (char *error) {
			delete error;
			
			if (expresion_numero != NULL) {
				remover_produccion(expresion_numero);
				expresion_numero = NULL;
			}
			if (operador2 != NULL) {
				remover_produccion(operador2);
				operador2 = NULL;
			}
			if (expresion_numero2 != NULL) {
				remover_produccion(expresion_numero2);
				expresion_numero2 = NULL;
			}
			
			resetear_posicion(contexto);
		}
		
		if (expresion_string == NULL && expresion_numero == NULL) {
			match = false;
		} else if (ancho_produccion(expresion_string) + ancho_produccion(operador1) + ancho_produccion(expresion_string2) >
					ancho_produccion(expresion_numero) + ancho_produccion(operador2) + ancho_produccion(expresion_numero2)) {
			adelantar_posicion(contexto, expresion_string);
			adelantar_posicion(contexto, operador1);
			adelantar_posicion(contexto, expresion_string2);
			
			agregar_hijo(factor_condicion, expresion_string);
			agregar_hijo(factor_condicion, operador1);
			agregar_hijo(factor_condicion, expresion_string2);

			if (expresion_numero != NULL) {
				remover_produccion(expresion_numero);
				expresion_numero = NULL;
			}
			if (operador2 != NULL) {
				remover_produccion(operador2);
				operador2 = NULL;
			}
			if (expresion_numero2 != NULL) {
				remover_produccion(expresion_numero2);
				expresion_numero2 = NULL;
			}
			
			match = true;
		} else {
			adelantar_posicion(contexto, expresion_numero);
			adelantar_posicion(contexto, operador2);
			adelantar_posicion(contexto, expresion_numero2);

			agregar_hijo(factor_condicion, expresion_numero);
			agregar_hijo(factor_condicion, operador2);
			agregar_hijo(factor_condicion, expresion_numero2);

			if (expresion_string != NULL) {
				remover_produccion(expresion_string);
				expresion_string = NULL;
			}
			if (operador1 != NULL) {
				remover_produccion(operador1);
				operador1 = NULL;
			}
			if (expresion_string2 != NULL) {
				remover_produccion(expresion_string2);
				expresion_string2 = NULL;
			}
			
			match = true;
		}
		
		if (match == false) {
			LPAR = consumir_token(contexto, TOKEN_LPAR);
			agregar_hijo(factor_condicion, LPAR);
			
			condicion = produccion_condicion(contexto);
			agregar_hijo(factor_condicion, condicion);
			
			RPAR = consumir_token(contexto, TOKEN_RPAR);
			agregar_hijo(factor_condicion, RPAR);
		}
	} catch (char *error) {
		if (factor_condicion != NULL) {
			remover_produccion(factor_condicion);
		}
		
		throw error;
	}
	
	return factor_condicion;
}

PRODUCCION* produccion_condicion(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <condicion> ::= <factor condicion> [ <condicion rr> ] 
	PRODUCCION *condicion, *factor_condicion, *condicion_rr;
	
	try {
		condicion = crear_produccion(PRODUCCION_CONDICION);
		
		factor_condicion = produccion_factor_condicion(contexto);
		agregar_hijo(condicion, factor_condicion);
		
		marcar_posicion(contexto);
		try {
			condicion_rr = produccion_condicion_rr(contexto);
			agregar_hijo(condicion, condicion_rr);
			
			cancelar_posicion(contexto);
		} catch (char *error) {
			delete error;
			
			resetear_posicion(contexto);
		}
	} catch (char *error) {
		if (condicion != NULL) {
			remover_produccion(condicion);
		}
		
		throw error;
	}
	
	return condicion;
}

PRODUCCION* produccion_condicion_rr(CONTEXTO *contexto) {
	assert(contexto != NULL);
	
	// <condicion rr> ::= <union> <factor condicion> [ <condicion rr> ]
	PRODUCCION* condicion_rr, *unions, *factor_condicion, *condicion_rrs;
	
	try {
		condicion_rr = crear_produccion(PRODUCCION_CONDICION_RR);
		
		unions = produccion_union(contexto);
		agregar_hijo(condicion_rr, unions);
		
		factor_condicion = produccion_factor_condicion(contexto);
		agregar_hijo(condicion_rr, factor_condicion);
		
		marcar_posicion(contexto);
		try {
			condicion_rrs = produccion_condicion_rr(contexto);
			agregar_hijo(condicion_rr, condicion_rrs);
			
			cancelar_posicion(contexto);
		} catch (char *error) {
			delete error;
			
			resetear_posicion(contexto);
		}
	} catch (char *error) {
		if (condicion_rr != NULL) {
			remover_produccion(condicion_rr);
		}
		
		throw error;
	}
	
	return condicion_rr;
}

PRODUCCION* produccion_union(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <union> ::= "&&" | "||"
	PRODUCCION *unions, *AND, *OR;
	
	try {
		unions = crear_produccion(PRODUCCION_UNION);
		
		if (token_in_first(contexto, TOKEN_AND) == true) {
			AND = consumir_token(contexto, TOKEN_AND);
			agregar_hijo(unions, AND);
		} else if (token_in_first(contexto, TOKEN_OR) == true) {
			OR = consumir_token(contexto, TOKEN_OR);
			agregar_hijo(unions, OR);
		} else {
			throw_token_error(contexto, "se esperaba: &&, || (union)");
		}
	} catch (char *error) {
		if (unions != NULL) {
			remover_produccion(unions);
		}
		
		throw error;
	}
	
	return unions;
}

PRODUCCION* produccion_operador(CONTEXTO* contexto) {
	assert(contexto != NULL);

	// <operador> ::= "==" | ">=" | "<=" | "!=" | ">" | "<"
	PRODUCCION *operador, *EQ, *GE, *LE, *NE, *GT, *LT;
	
	try {
		operador = crear_produccion(PRODUCCION_OPERADOR);
		
		if (token_in_first(contexto, TOKEN_EQ) == true) {
			EQ = consumir_token(contexto, TOKEN_EQ);
			agregar_hijo(operador, EQ);
		} else if (token_in_first(contexto, TOKEN_GE) == true) {
			GE = consumir_token(contexto, TOKEN_GE);
			agregar_hijo(operador, GE);
		} else if (token_in_first(contexto, TOKEN_LE) == true) {
			LE = consumir_token(contexto, TOKEN_LE);
			agregar_hijo(operador, LE);
		} else if (token_in_first(contexto, TOKEN_NE) == true) {
			NE = consumir_token(contexto, TOKEN_NE);
			agregar_hijo(operador, NE);
		} else if (token_in_first(contexto, TOKEN_GT) == true) {
			GT = consumir_token(contexto, TOKEN_GT);
			agregar_hijo(operador, GT);
		} else if (token_in_first(contexto, TOKEN_LT) == true) {
			LT = consumir_token(contexto, TOKEN_LT);
			agregar_hijo(operador, LT);
		} else {
			throw_token_error(contexto, "se esperaba: ==, !=, >, >=, <, <= (operador)");
		}
	} catch (char *error) {
		if (operador != NULL) {
			remover_produccion(operador);
		}
		
		throw error;
	}
	
	return operador;
}

bool tif_declaracion_arreglo(CONTEXTO* contexto) {
	return token_in_first(contexto, TOKEN_INTEGER) ||
			token_in_first(contexto, TOKEN_CHAR) ||
			token_in_first(contexto, TOKEN_DOUBLE);
}

bool tif_declaracion_variable(CONTEXTO* contexto) {
	return token_in_first(contexto, TOKEN_INTEGER) ||
			token_in_first(contexto, TOKEN_CHAR) ||
			token_in_first(contexto, TOKEN_DOUBLE);
}

bool tif_instrucciones(CONTEXTO* contexto) {
	return token_in_first(contexto, TOKEN_ETIQUETA) ||
			token_in_first(contexto, TOKEN_READ) ||
			token_in_first(contexto, TOKEN_PRINT) ||
			token_in_first(contexto, TOKEN_PRINTLN) ||
			token_in_first(contexto, TOKEN_IF) ||
			token_in_first(contexto, TOKEN_WHILE) ||
			token_in_first(contexto, TOKEN_FOR);
}

bool tif_expresion(CONTEXTO* contexto) {
	return token_in_first(contexto, TOKEN_STRING) ||
			token_in_first(contexto, TOKEN_ADD) ||
			token_in_first(contexto, TOKEN_LPAR) ||
			token_in_first(contexto, TOKEN_NUMERO) ||
			token_in_first(contexto, TOKEN_ENTERO) ||
			token_in_first(contexto, TOKEN_ETIQUETA) ||
			token_in_first(contexto, TOKEN_REST);
}

bool tif_expresion_string(CONTEXTO* contexto) {
	return token_in_first(contexto, TOKEN_STRING) ||
			token_in_first(contexto, TOKEN_LPAR) ||
			token_in_first(contexto, TOKEN_ETIQUETA);
}

bool tif_factor(CONTEXTO* contexto) {
	return token_in_first(contexto, TOKEN_ENTERO) ||
			token_in_first(contexto, TOKEN_NUMERO) ||
			token_in_first(contexto, TOKEN_ETIQUETA) ||
			token_in_first(contexto, TOKEN_LPAR) ||
			token_in_first(contexto, TOKEN_ADD) ||
			token_in_first(contexto, TOKEN_REST);
}

void debug_produccion(PRODUCCION *produccion) {
	DEBUG_TABLE debug_table;
	
	debug_table[PRODUCCION_ASIGNACION] = "ASIGNACION";
	debug_table[PRODUCCION_CH] = "CH";
	debug_table[PRODUCCION_CONDICION] = "CONDICION";
	debug_table[PRODUCCION_CONDICION_RR] = "CONDICION_RR";
	debug_table[PRODUCCION_DATOS] = "DATOS";
	debug_table[PRODUCCION_DECLARACION_ARREGLO] = "DECLARACION_ARREGLO";
	debug_table[PRODUCCION_DECLARACION_VARIABLE] = "DECLARACION_VARIABLE";
	debug_table[PRODUCCION_ESCRITURA] = "ESCRITURA";
	debug_table[PRODUCCION_ETIQUETA] = "ETIQUETA";
	debug_table[PRODUCCION_EXPRESION] = "EXPRESION";
	debug_table[PRODUCCION_EXPRESION_NUMERO] = "EXPRESION_NUMERO";
	debug_table[PRODUCCION_EXPRESION_STRING] = "EXPRESION_STRING";
	debug_table[PRODUCCION_EXPRESION_NUMERO_RR] = "EXPRESION_NUMERO_RR";
	debug_table[PRODUCCION_EXPRESION_STRING_RR] = "EXPRESION_STRING_RR";
	debug_table[PRODUCCION_FACTOR] = "FACTOR";
	debug_table[PRODUCCION_FACTOR_CONDICION] = "FACTOR_CONDICION";
	debug_table[PRODUCCION_FOR] = "FOR";
	debug_table[PRODUCCION_IF] = "IF";
	debug_table[PRODUCCION_INSTRUCCION] = "INSTRUCCION";
	debug_table[PRODUCCION_INSTRUCCIONES] = "INSTRUCCIONES";
	debug_table[PRODUCCION_LECTURA] = "LECTURA";
	debug_table[PRODUCCION_OPERADOR] = "OPERADOR";
	debug_table[PRODUCCION_PRINT] = "PRINT";
	debug_table[PRODUCCION_TERMINO] = "TERMINO";
	debug_table[PRODUCCION_TERMINO_RR] = "TERMINO_RR";
	debug_table[PRODUCCION_TIPO_DATO] = "TIPO_DATO";
	debug_table[PRODUCCION_LEXEMA] = "TOKEN";
	debug_table[PRODUCCION_UNION] = "UNION";
	debug_table[PRODUCCION_WHILE] = "WHILE";
	debug_table[PRODUCCION_STRING] = "STRING";
	
	debug_produccion(produccion, 0, debug_table);
}

void debug_produccion(PRODUCCION *produccion, int nivel, DEBUG_TABLE table) {
	string spaces = "";
	for (int i = 0; i < nivel; i++) {
		spaces += "__";
	}
	
	if (produccion->tipo != PRODUCCION_LEXEMA) {
		cout << spaces << "==S> " << table[produccion->tipo] << endl;
		
		PRODUCCION *hijo = produccion->hijos;
		while (hijo != NULL) {
			debug_produccion(hijo, nivel + 1, table);
			hijo = hijo->siguiente;
		}
	} else {
		cout << spaces << " " << produccion->token->token << endl;
	}
}

PRODUCCION* seleccionar_produccion(PRODUCCION *produccion, const char* camino) {
	assert(produccion != NULL);
	PRODUCCION* actual = produccion;
	
	char c =  ' ';
	unsigned int idx = 0;
	while ((c = camino[idx++]) != 0 && actual != NULL) {
		assert(c == '.' || c == '>');
		
		if (c == '.') {
			actual = actual->hijos;
		} else if (c == '>') {
			actual = actual->siguiente;
		}
	}

	return actual;
}

TOKEN* seleccionar_token(PRODUCCION *produccion, const char* camino) {
	PRODUCCION *actual = seleccionar_produccion(produccion, camino);
	
	if (actual == NULL) {
		return NULL;
	} else {
		return actual->token;
	}
}
