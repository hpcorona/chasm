#include "gc_chvm.h"

void generar_codigo_chvm(PRODUCCION* ch, TABLA_SIMBOLOS* simbolos, string salida) {
	PROGRAMA* programa = ch_crear(salida, simbolos);
	ch_generar_ch(ch, programa);
	ch_cerrar(programa);
	
	delete programa;
}

PROGRAMA* ch_crear(string nombre, TABLA_SIMBOLOS* simbolos) {
	assert(simbolos != NULL);
	
	PROGRAMA *programa = new PROGRAMA();
	
	programa->segmentoCodigo = 0;
	programa->segmentoDatos = 0;
	
	programa->file = fopen(nombre.c_str(), "w+b");
	if (programa->file == NULL) {
		return NULL;
	}
	
	programa->simbolos = simbolos;
	
	programa->direccionesCount = 0;
	
	ch_escribir_encabezado(programa);
	
	return programa;
}

void ch_escribir_encabezado(PROGRAMA* programa) {
	assert(programa != NULL);
	assert(programa->file != NULL);
	
	fprintf(programa->file, "(C)ChAsmICC1");
	
	programa->segmentoDatos = 0;
	programa->segmentoCodigo = 0;
	
	TABLA_SIMBOLOS::iterator iterador;
	
	for (iterador = programa->simbolos->begin(); iterador != programa->simbolos->end(); iterador++) {
		SIMBOLO* simbolo = iterador->second;
		
		simbolo->direccion = programa->segmentoDatos;
		
		short baseSize = 1;
		switch(simbolo->tipo) {
		case DATO_INTEGER:
			baseSize = 4;
			break;
		case DATO_CHAR:
			baseSize = 1;
			break;
		case DATO_DOUBLE:
			baseSize = 8;
			break;
		}
		
		if (simbolo->dim1 != 0) {
			baseSize *= simbolo->dim1;
		}
		
		if (simbolo->dim2 != 0) {
			baseSize *= simbolo->dim2;
		}
		
		programa->segmentoDatos += baseSize;
	}
	fwrite(&(programa->segmentoDatos), sizeof(short), 2, programa->file);
}

void ch_cerrar(PROGRAMA* programa) {
	assert(programa != NULL);
	assert(programa->file != NULL);
	
	ch_escribir_tamanio_codigo(programa);
	
	fclose(programa->file);
	programa->file = NULL;
}

void ch_escribir_tamanio_codigo(PROGRAMA* programa) {
	assert(programa != NULL);
	assert(programa->file != NULL);
	
	fseek(programa->file, 14, SEEK_SET);
	fwrite(&(programa->segmentoCodigo), sizeof(short), 1, programa->file);
}

void ch_embedd_code_address(PROGRAMA* programa, short direccion) {
	assert(programa != NULL);
	assert(programa->file != NULL);

	fwrite(&direccion, sizeof(short), 1, programa->file);
	
	programa->segmentoCodigo += sizeof(short);
}

void ch_embedd_code_address_in_placeholder(PROGRAMA* programa, short direccion) {
	assert(programa != NULL);
	assert(programa->file != NULL);

	short address = programa->direcciones[--programa->direccionesCount];

	fseek(programa->file, 16 + address, SEEK_SET);
	fwrite(&direccion, sizeof(short), 1, programa->file);
	
	fseek(programa->file, 16 + programa->segmentoCodigo, SEEK_SET);
}

void ch_embedd_code_address_placeholder(PROGRAMA* programa) {
	assert(programa != NULL);
	assert(programa->file != NULL);

	programa->direcciones[programa->direccionesCount++] = programa->segmentoCodigo;
	ch_embedd_code_address(programa, 0);
}

void ch_embedd_data_address(PROGRAMA* programa, PRODUCCION *etiqueta) {
	assert(programa != NULL);
	assert(programa->simbolos != NULL);
	
	TOKEN *token_etiqueta = seleccionar_token(etiqueta, ".");
	assert(token_etiqueta != NULL);
	
	SIMBOLO* simbolo = (*(programa->simbolos))[token_etiqueta->token];
	assert(simbolo != NULL);
	
	short direccion = (short)simbolo->direccion;
	
	ch_embedd_code_address(programa, direccion);
}

void ch_insertar_instruccion(PROGRAMA* programa, char codigo_instruccion) {
	ch_embedd_char(programa, codigo_instruccion);
}

void ch_embedd_string(PROGRAMA* programa, TOKEN *strings) {
	assert(programa != NULL);
	assert(programa->file != NULL);
	assert(strings != NULL);
	assert(strings->tipo == TOKEN_STRING);
	
	string token = strings->token;
	
	char* dir = (char*)token.c_str();
	dir++;

	char c = (char)token.length() - 2;
	
	fwrite(&c, sizeof(char), 1, programa->file);
	fwrite(dir, sizeof(char), token.length() - 2, programa->file);

	programa->segmentoCodigo += token.length() - 1;
}

void ch_embedd_char(PROGRAMA* programa, char c) {
	assert(programa != NULL);
	assert(programa->file != NULL);
	
	fwrite(&c, sizeof(char), 1, programa->file);
	
	programa->segmentoCodigo += sizeof(char);
}

void ch_embedd_integer(PROGRAMA* programa, int i) {
	assert(programa != NULL);
	assert(programa->file != NULL);
	
	fwrite(&i, sizeof(int), 1, programa->file);
	
	programa->segmentoCodigo += sizeof(int);
}

void ch_embedd_double(PROGRAMA* programa, double d) {
	assert(programa != NULL);
	assert(programa->file != NULL);
	
	fwrite(&d, sizeof(double), 1, programa->file);
	
	programa->segmentoCodigo += sizeof(double);
}

void ch_generar_ch(PRODUCCION *ch, PROGRAMA* programa) {
	assert(ch != NULL);
	assert(ch->tipo == PRODUCCION_CH);
	
	ch_generar_hijos(ch, programa);
}

void ch_generar_hijos(PRODUCCION *produccion_padre, PROGRAMA* programa) {
	assert(produccion_padre != NULL);
	
	PRODUCCION *hijo = produccion_padre->hijos;
	while (hijo != NULL) {
		ch_generar_produccion(hijo, programa);
		
		hijo = hijo->siguiente;
	}
}

void ch_generar_produccion(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	TOKEN* token = NULL;
	
	switch(produccion->tipo) {
	case PRODUCCION_DATOS:
	case PRODUCCION_DECLARACION_ARREGLO:
	case PRODUCCION_DECLARACION_VARIABLE:
	case PRODUCCION_TIPO_DATO:
		// Los datos no generan codigo
		return;
	case PRODUCCION_INSTRUCCIONES:
	case PRODUCCION_INSTRUCCION:
	case PRODUCCION_EXPRESION:
		ch_generar_hijos(produccion, programa);
		break;
	case PRODUCCION_ASIGNACION:
		ch_generar_produccion_asignacion(produccion, programa);
		break;
	case PRODUCCION_LECTURA:
		ch_generar_produccion_lectura(produccion, programa);
		break;
	case PRODUCCION_ESCRITURA:
		ch_generar_produccion_escritura(produccion, programa);
		break;
	case PRODUCCION_IF:
		ch_generar_produccion_if(produccion, programa);
		break;
	case PRODUCCION_WHILE:
		ch_generar_produccion_while(produccion, programa);
		break;
	case PRODUCCION_FOR:
		ch_generar_produccion_for(produccion, programa);
		break;
	case PRODUCCION_ETIQUETA:
		ch_generar_produccion_etiqueta(produccion, programa);
		break;
	case PRODUCCION_EXPRESION_STRING:
		ch_generar_produccion_expresion_string(produccion, programa);
		break;
	case PRODUCCION_EXPRESION_STRING_RR:
		ch_generar_produccion_expresion_string_rr(produccion, programa);
		break;
	case PRODUCCION_EXPRESION_NUMERO:
		ch_generar_produccion_expresion_numero(produccion, programa);
		break;
	case PRODUCCION_EXPRESION_NUMERO_RR:
		ch_generar_produccion_expresion_numero_rr(produccion, programa);
		break;
	case PRODUCCION_TERMINO:
		ch_generar_produccion_termino(produccion, programa);
		break;
	case PRODUCCION_TERMINO_RR:
		ch_generar_produccion_termino_rr(produccion, programa);
		break;
	case PRODUCCION_FACTOR:
		ch_generar_produccion_factor(produccion, programa);
		break;
	case PRODUCCION_CONDICION:
		ch_generar_produccion_condicion(produccion, programa);
		break;
	case PRODUCCION_CONDICION_RR:
		ch_generar_produccion_condicion_rr(produccion, programa);
		break;
	case PRODUCCION_FACTOR_CONDICION:
		ch_generar_produccion_factor_condicion(produccion, programa);
		break;
	case PRODUCCION_STRING:
		ch_generar_produccion_string(produccion, programa);
		break;
	case PRODUCCION_LEXEMA:
		token = produccion->token;
		switch(token->tipo) {
		case TOKEN_START:
			break;
		case TOKEN_END:
		case TOKEN_STOP:
			ch_insertar_instruccion(programa, CH_HLT);
			break;
		default:
			assert(0);
		}
		break;
	default:
		assert(0);
		break;
	}	
}

void ch_generar_produccion_asignacion(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_ASIGNACION);
	assert(programa != NULL);
	assert(programa->simbolos != NULL);
	
	PRODUCCION *expresion = seleccionar_produccion(produccion, ".>>");
	assert(expresion != NULL);
	assert(expresion->tipo == PRODUCCION_EXPRESION);
	
	ch_generar_produccion(expresion, programa);
	
	PRODUCCION *etiqueta = seleccionar_produccion(produccion, ".");
	assert(etiqueta != NULL);
	assert(etiqueta->tipo == PRODUCCION_ETIQUETA);
	
	TOKEN* token_etiqueta = seleccionar_token(etiqueta, ".");
	assert(token_etiqueta != NULL);
	
	SIMBOLO* dato = (*(programa->simbolos))[token_etiqueta->token];
	assert(dato != NULL);
	
	TIPO_RUNTIME tipo = tipo_etiqueta(programa->simbolos, etiqueta);
	
	PRODUCCION *corchete = seleccionar_produccion(etiqueta, ".>");
	if (corchete != NULL) {
		ch_generar_produccion(etiqueta, programa);
		if (tipo == RUNTIME_STRING) {
			assert(dato->dim1 != 0);
			ch_insertar_instruccion(programa, CH_PUSHCI);
			ch_embedd_integer(programa, dato->dim1);
			ch_insertar_instruccion(programa, CH_POPZ);
			
			ch_insertar_instruccion(programa, CH_POPVM);
		} else {
			switch (dato->tipo) {
			case DATO_INTEGER:
				ch_insertar_instruccion(programa, CH_POPVI);
				break;
			case DATO_CHAR:
				ch_insertar_instruccion(programa, CH_POPVC);
				break;
			case DATO_DOUBLE:
				ch_insertar_instruccion(programa, CH_POPVD);
				break;
			default:
				assert(0);
				break;
			}
		}
	} else {
		if (tipo == RUNTIME_STRING) {
			ch_insertar_instruccion(programa, CH_POPM);
		} else {
			switch (dato->tipo) {
			case DATO_INTEGER:
				ch_insertar_instruccion(programa, CH_POPI);
				break;
			case DATO_CHAR:
				ch_insertar_instruccion(programa, CH_POPC);
				break;
			case DATO_DOUBLE:
				ch_insertar_instruccion(programa, CH_POPD);
				break;
			default:
				assert(0);
				break;
			}
		}
	}
	
	ch_embedd_data_address(programa, etiqueta);
}

void ch_generar_produccion_lectura(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_LECTURA);
	assert(programa != NULL);
	assert(programa->simbolos != NULL);
	
	PRODUCCION *etiqueta = seleccionar_produccion(produccion, ".>>");
	assert(etiqueta != NULL);
	assert(etiqueta->tipo == PRODUCCION_ETIQUETA);
	
	TOKEN* token_etiqueta = seleccionar_token(etiqueta, ".");
	assert(token_etiqueta != NULL);
	
	SIMBOLO* dato = (*(programa->simbolos))[token_etiqueta->token];
	assert(dato != NULL);
	
	TIPO_RUNTIME tipo = tipo_etiqueta(programa->simbolos, etiqueta);
	PRODUCCION *corchete = seleccionar_produccion(etiqueta, ".>");
	if (corchete != NULL) {
		ch_generar_produccion(etiqueta, programa);
		if (tipo == RUNTIME_STRING) {
			assert(dato->dim1 != 0);
			ch_insertar_instruccion(programa, CH_PUSHCI);
			ch_embedd_integer(programa, dato->dim1);
			ch_insertar_instruccion(programa, CH_POPZ);
			
			ch_insertar_instruccion(programa, CH_READVS);
		} else {
			switch (dato->tipo) {
			case DATO_INTEGER:
				ch_insertar_instruccion(programa, CH_READVI);
				break;
			case DATO_CHAR:
				ch_insertar_instruccion(programa, CH_READVC);
				break;
			case DATO_DOUBLE:
				ch_insertar_instruccion(programa, CH_READVD);
				break;
			default:
				assert(0);
				break;
			}
		}
	} else {
		if (tipo == RUNTIME_STRING) {
			ch_insertar_instruccion(programa, CH_READS);
		} else {
			switch (dato->tipo) {
			case DATO_INTEGER:
				ch_insertar_instruccion(programa, CH_READI);
				break;
			case DATO_CHAR:
				ch_insertar_instruccion(programa, CH_READC);
				break;
			case DATO_DOUBLE:
				ch_insertar_instruccion(programa, CH_READD);
				break;
			default:
				assert(0);
				break;
			}
		}
	}
		
	ch_embedd_data_address(programa, etiqueta);
}

void ch_generar_produccion_escritura(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_ESCRITURA);
	
	PRODUCCION *expresion = seleccionar_produccion(produccion, ".>>");
	assert(expresion != NULL);
	assert(expresion->tipo == PRODUCCION_EXPRESION);
	
	ch_generar_produccion(expresion, programa);
	
	TOKEN *print = seleccionar_token(produccion, "..");
	assert(print != NULL);
	assert(print->tipo == TOKEN_PRINT || print->tipo == TOKEN_PRINTLN);
	
	ch_insertar_instruccion(programa, CH_WRITEP);
	
	if (print->tipo == TOKEN_PRINTLN) {
		ch_insertar_instruccion(programa, CH_WRITECR);
	}
}

void ch_generar_produccion_if(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_IF);
	
	PRODUCCION* condicion = seleccionar_produccion(produccion, ".>>");
	assert(condicion != NULL);
	assert(condicion->tipo == PRODUCCION_CONDICION);
	
	ch_generar_produccion(condicion, programa);
	
	ch_insertar_instruccion(programa, CH_JMPF);
	
	ch_embedd_code_address_placeholder(programa);
	
	PRODUCCION* instrucciones = seleccionar_produccion(produccion, ".>>>>");
	assert(instrucciones != NULL);
	assert(instrucciones->tipo == PRODUCCION_INSTRUCCIONES);
	
	ch_generar_produccion(instrucciones, programa);
	
	TOKEN* elses = seleccionar_token(produccion, ".>>>>>");
	assert(elses != NULL);
	assert(elses->tipo == TOKEN_ELSE || elses->tipo == TOKEN_ENDIF);
	
	if (elses->tipo == TOKEN_ELSE) {
		ch_embedd_code_address_in_placeholder(programa, programa->segmentoCodigo + 3);
		
		ch_insertar_instruccion(programa, CH_JMP);
		ch_embedd_code_address_placeholder(programa);
		
		instrucciones = seleccionar_produccion(produccion, ".>>>>>>");
		assert(instrucciones != NULL);
		assert(instrucciones->tipo == PRODUCCION_INSTRUCCIONES);
		
		ch_generar_produccion(instrucciones, programa);
	}

	ch_embedd_code_address_in_placeholder(programa, programa->segmentoCodigo);
}

void ch_generar_produccion_while(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_WHILE);
	
	short direccion = programa->segmentoCodigo;
	
	PRODUCCION* condicion = seleccionar_produccion(produccion, ".>>");
	assert(condicion != NULL);
	assert(condicion->tipo == PRODUCCION_CONDICION);
	
	ch_generar_produccion(condicion, programa);
	
	ch_insertar_instruccion(programa, CH_JMPF);
	
	ch_embedd_code_address_placeholder(programa);
	
	PRODUCCION* instrucciones = seleccionar_produccion(produccion, ".>>>>");
	assert(instrucciones != NULL);
	assert(instrucciones->tipo == PRODUCCION_INSTRUCCIONES);
	
	ch_generar_produccion(instrucciones, programa);
	
	ch_insertar_instruccion(programa, CH_JMP);
	ch_embedd_code_address(programa, direccion);
	
	ch_embedd_code_address_in_placeholder(programa, programa->segmentoCodigo);
}

void ch_generar_produccion_for(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_FOR);
	
	PRODUCCION* asignacion = seleccionar_produccion(produccion, ".>>");
	assert(asignacion != NULL);
	assert(asignacion->tipo == PRODUCCION_ASIGNACION);
	
	ch_generar_produccion(asignacion, programa);
	
	short direccion = programa->segmentoCodigo;
	
	PRODUCCION* condicion = seleccionar_produccion(produccion, ".>>>>");
	assert(condicion != NULL);
	assert(condicion->tipo == PRODUCCION_CONDICION);
	
	ch_generar_produccion(condicion, programa);
	
	ch_insertar_instruccion(programa, CH_JMPF);
	
	ch_embedd_code_address_placeholder(programa);
	
	PRODUCCION* instrucciones = seleccionar_produccion(produccion, ".>>>>>>>>");
	assert(instrucciones != NULL);
	assert(instrucciones->tipo == PRODUCCION_INSTRUCCIONES);
	
	ch_generar_produccion(instrucciones, programa);

	asignacion = seleccionar_produccion(produccion, ".>>>>>>");
	assert(asignacion != NULL);
	assert(asignacion->tipo == PRODUCCION_ASIGNACION);
	
	ch_generar_produccion(asignacion, programa);
	
	ch_insertar_instruccion(programa, CH_JMP);
	ch_embedd_code_address(programa, direccion);
	
	ch_embedd_code_address_in_placeholder(programa, programa->segmentoCodigo);
}

//este solo genera la instruccion para el POPX
void ch_generar_produccion_etiqueta(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_ETIQUETA);
	
	TOKEN* token_etiqueta = seleccionar_token(produccion, ".");
	assert(token_etiqueta != NULL);
	
	PRODUCCION* exprDim1 = seleccionar_produccion(produccion, ".>>");
	PRODUCCION* exprDim2 = seleccionar_produccion(produccion, ".>>>>");
	
	if (exprDim1 == NULL) {
		assert(exprDim2 == NULL);
		
		// no hacer nada :)
	} else {
		if (exprDim2 == NULL) {
			// es una variable asi: var[n]
			assert(exprDim1->tipo == PRODUCCION_EXPRESION_NUMERO);
			
			ch_generar_produccion(exprDim1, programa);
			
			ch_insertar_instruccion(programa, CH_POPX);
		} else {
			// es una variable asi: var[n,m]
			assert(exprDim1->tipo == PRODUCCION_EXPRESION_NUMERO);
			assert(exprDim2->tipo == PRODUCCION_EXPRESION_NUMERO);
			
			SIMBOLO* variable = (*(programa->simbolos))[token_etiqueta->token];
			assert(variable != NULL);

			ch_generar_produccion(exprDim1, programa);
			ch_embedd_integer(programa, variable->dim1);
			ch_insertar_instruccion(programa, CH_MUL);
			
			ch_generar_produccion(exprDim2, programa);
			ch_insertar_instruccion(programa, CH_ADD);
			
			ch_insertar_instruccion(programa, CH_POPX);
		}
	}
}

void ch_generar_produccion_expresion_string(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_EXPRESION_STRING);
	
	PRODUCCION* strings = seleccionar_produccion(produccion, ".");
	assert(strings != NULL);
	assert(strings->tipo == PRODUCCION_STRING);
	
	ch_generar_produccion(strings, programa);
	
	PRODUCCION* rr = seleccionar_produccion(produccion, ".>");
	if (rr != NULL) {
		assert(rr->tipo == PRODUCCION_EXPRESION_STRING_RR);
		
		ch_generar_produccion(rr, programa);
	}
}

void ch_generar_produccion_expresion_string_rr(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_EXPRESION_STRING_RR);
	
	PRODUCCION* concat = seleccionar_produccion(produccion, ".>");
	assert(concat != NULL);
	assert(concat->tipo == PRODUCCION_STRING || concat->tipo == PRODUCCION_EXPRESION_NUMERO);
	
	ch_generar_produccion(concat, programa);
	
	ch_insertar_instruccion(programa, CH_ADD);
	
	PRODUCCION* rr = seleccionar_produccion(produccion, ".>>");
	if (rr != NULL) {
		assert(rr->tipo == PRODUCCION_EXPRESION_STRING_RR);
		
		ch_generar_produccion(rr, programa);
	}
}

void ch_generar_produccion_expresion_numero(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_EXPRESION_NUMERO);
	
	PRODUCCION* termino = seleccionar_produccion(produccion, ".");
	assert(termino != NULL);
	assert(termino->tipo == PRODUCCION_TERMINO);
	
	ch_generar_produccion(termino, programa);
	
	PRODUCCION* rr = seleccionar_produccion(produccion, ".>");
	if (rr != NULL) {
		assert(rr->tipo == PRODUCCION_EXPRESION_NUMERO_RR);
		
		ch_generar_produccion(rr, programa);
	}	
}

void ch_generar_produccion_expresion_numero_rr(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_EXPRESION_NUMERO_RR);
	
	PRODUCCION* termino = seleccionar_produccion(produccion, ".>");
	assert(termino != NULL);
	assert(termino->tipo == PRODUCCION_TERMINO);
	
	ch_generar_produccion(termino, programa);
	
	TOKEN* operacion = seleccionar_token(produccion, ".");
	assert(operacion != NULL);
	assert(operacion->tipo == TOKEN_ADD || operacion->tipo == TOKEN_REST);
	
	if (operacion->tipo == TOKEN_ADD) {
		ch_insertar_instruccion(programa, CH_ADD);
	} else {
		ch_insertar_instruccion(programa, CH_SUB);
	}
	
	PRODUCCION* rr = seleccionar_produccion(produccion, ".>>");
	if (rr != NULL) {
		assert(rr->tipo == PRODUCCION_EXPRESION_NUMERO_RR);
		
		ch_generar_produccion(rr, programa);
	}	
}

void ch_generar_produccion_termino(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_TERMINO);
	
	PRODUCCION* factor = seleccionar_produccion(produccion, ".");
	assert(factor != NULL);
	assert(factor->tipo == PRODUCCION_FACTOR);
	
	ch_generar_produccion(factor, programa);
	
	PRODUCCION* rr = seleccionar_produccion(produccion, ".>");
	if (rr != NULL) {
		assert(rr->tipo == PRODUCCION_TERMINO_RR);
		
		ch_generar_produccion(rr, programa);
	}	
}

void ch_generar_produccion_termino_rr(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_TERMINO);
	
	PRODUCCION* factor = seleccionar_produccion(produccion, ".>");
	assert(factor != NULL);
	assert(factor->tipo == PRODUCCION_FACTOR);
	
	ch_generar_produccion(factor, programa);

	TOKEN* operacion = seleccionar_token(produccion, ".");
	assert(operacion != NULL);
	assert(operacion->tipo == TOKEN_MUL || operacion->tipo == TOKEN_DIV || operacion->tipo == TOKEN_MOD);
	
	if (operacion->tipo == TOKEN_MUL) {
		ch_insertar_instruccion(programa, CH_MUL);
	} else if (operacion->tipo == TOKEN_DIV) {
		ch_insertar_instruccion(programa, CH_DIV);
	} else {
		ch_insertar_instruccion(programa, CH_MOD);
	}
	
	PRODUCCION* rr = seleccionar_produccion(produccion, ".>>");
	if (rr != NULL) {
		assert(rr->tipo == PRODUCCION_TERMINO_RR);
		
		ch_generar_produccion(rr, programa);
	}	
}

void ch_generar_produccion_factor(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_FACTOR);
	
	bool resta = false;
	PRODUCCION* var = NULL;
	
	var = seleccionar_produccion(produccion, ".");
	assert(var != NULL);
	
	if (var->tipo == PRODUCCION_LEXEMA) {
		TOKEN* token = var->token;
		if (token->tipo == TOKEN_LPAR) {
			var = seleccionar_produccion(produccion, ".>");
			assert(var != NULL);
			assert(var->tipo == PRODUCCION_EXPRESION_NUMERO);
			
			ch_generar_produccion(var, programa);
			return;
		} else if (token->tipo == TOKEN_REST) {
			resta = true;
			
			var = seleccionar_produccion(produccion, ".>");
		}
	}
	
	assert(var != NULL);
	assert(var->tipo == PRODUCCION_LEXEMA || var->tipo == PRODUCCION_ETIQUETA);
	
	if (resta == true) {
		ch_embedd_char(programa, 0);
	}
	
	if (var->tipo == PRODUCCION_LEXEMA) {
		TOKEN* token = var->token;
		//char w;
		int x;
		float y;
		
		switch(token->tipo) {
		case TOKEN_ENTERO:
			x = atoi(token->token.c_str());
			ch_insertar_instruccion(programa, CH_PUSHCI);
			ch_embedd_integer(programa, x);
			break;
		case TOKEN_NUMERO:
			y = atof(token->token.c_str());
			ch_insertar_instruccion(programa, CH_PUSHCD);
			ch_embedd_double(programa, y);
			break;
		default:
			assert(0);
			break;
		}
	} else {
		ch_generar_produccion(var, programa);
		
		TOKEN* etiqueta = seleccionar_token(var, ".");
		assert(etiqueta != NULL);
		
		SIMBOLO* variable = (*(programa->simbolos))[etiqueta->token];
		assert(variable != NULL);
		
		PRODUCCION* corchete = seleccionar_produccion(var, ".>");
		
		if (corchete != NULL) {
			assert(var->tipo == PRODUCCION_ETIQUETA);
			ch_generar_produccion(var, programa);
			switch(variable->tipo) {
			case DATO_INTEGER:
				ch_insertar_instruccion(programa, CH_PUSHVI);
				break;
			case DATO_CHAR:
				ch_insertar_instruccion(programa, CH_PUSHVC);
				break;
			case DATO_DOUBLE:
				ch_insertar_instruccion(programa, CH_PUSHVD);
				break;
			default:
				assert(0);
				break;
			}
		} else {
			switch(variable->tipo) {
			case DATO_INTEGER:
				ch_insertar_instruccion(programa, CH_PUSHI);
				break;
			case DATO_CHAR:
				ch_insertar_instruccion(programa, CH_PUSHC);
				break;
			case DATO_DOUBLE:
				ch_insertar_instruccion(programa, CH_PUSHD);
				break;
			default:
				assert(0);
				break;
			}
		}
		
		ch_embedd_data_address(programa, var);
	}
	
	if (resta == true) {
		ch_insertar_instruccion(programa, CH_SUB);
	}
}

void ch_generar_produccion_condicion(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_CONDICION);
	
	PRODUCCION* factor_condicion = seleccionar_produccion(produccion, ".");
	assert(factor_condicion != NULL);
	assert(factor_condicion->tipo == PRODUCCION_FACTOR_CONDICION);
	
	ch_generar_produccion(factor_condicion, programa);
	
	PRODUCCION* rr = seleccionar_produccion(produccion, ".>");
	if (rr != NULL) {
		assert(rr->tipo == PRODUCCION_CONDICION_RR);
		
		ch_generar_produccion(rr, programa);
	}
}

void ch_generar_produccion_condicion_rr(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_CONDICION_RR);
	
	PRODUCCION* factor_condicion = seleccionar_produccion(produccion, ".>");
	assert(factor_condicion != NULL);
	assert(factor_condicion->tipo == PRODUCCION_FACTOR_CONDICION);
	
	ch_generar_produccion(factor_condicion, programa);
	
	PRODUCCION* unions = seleccionar_produccion(produccion, ".");
	assert(unions != NULL);
	assert(unions->tipo == PRODUCCION_UNION);
	
	TOKEN* lexema = seleccionar_token(produccion, "..");
	assert(lexema != NULL);
	assert(lexema->tipo == TOKEN_AND || lexema->tipo == TOKEN_OR);
	
	if (lexema->tipo == TOKEN_AND) {
		ch_insertar_instruccion(programa, CH_AND);
	} else {
		ch_insertar_instruccion(programa, CH_OR);
	}
	
	PRODUCCION* rr = seleccionar_produccion(produccion, ".>>");
	if (rr != NULL) {
		assert(rr->tipo == PRODUCCION_CONDICION_RR);
		
		ch_generar_produccion(rr, programa);
	}
}

void ch_generar_produccion_factor_condicion(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_FACTOR_CONDICION);
	
	PRODUCCION* operador = seleccionar_produccion(produccion, ".>");
	assert(operador != NULL);
	assert(operador->tipo == PRODUCCION_CONDICION || operador->tipo == PRODUCCION_OPERADOR);
	
	if (operador->tipo == PRODUCCION_CONDICION) {
		ch_generar_produccion(operador, programa);
	} else {
		PRODUCCION* expr1 = seleccionar_produccion(produccion, ".");
		PRODUCCION* expr2 = seleccionar_produccion(produccion, ".>>");
		assert(expr1 != NULL);
		assert(expr1->tipo == PRODUCCION_EXPRESION_NUMERO || expr1->tipo == PRODUCCION_EXPRESION_STRING);
		assert(expr2 != NULL);
		assert(expr2->tipo == PRODUCCION_EXPRESION_STRING || expr2->tipo == PRODUCCION_EXPRESION_NUMERO);
		
		ch_generar_produccion(expr1, programa);
		ch_generar_produccion(expr2, programa);
		
		TOKEN* token_operador = seleccionar_token(operador, ".");
		assert(token_operador != NULL);
		
		switch(token_operador->tipo) {
		case TOKEN_EQ:
			ch_insertar_instruccion(programa, CH_CMPEQ);
			break;
		case TOKEN_LT:
			ch_insertar_instruccion(programa, CH_CMPLT);
			break;
		case TOKEN_LE:
			ch_insertar_instruccion(programa, CH_CMPLE);
			break;
		case TOKEN_GT:
			ch_insertar_instruccion(programa, CH_CMPGT);
			break;
		case TOKEN_GE:
			ch_insertar_instruccion(programa, CH_CMPGE);
			break;
		case TOKEN_NE:
			ch_insertar_instruccion(programa, CH_CMPNE);
			break;
		default:
			assert(0);
			break;
		}
	}
}

void ch_generar_produccion_string(PRODUCCION *produccion, PROGRAMA* programa) {
	assert(produccion != NULL);
	assert(produccion->tipo == PRODUCCION_STRING);
	
	PRODUCCION* child = seleccionar_produccion(produccion, ".");
	assert(child != NULL);
	assert(child->tipo == PRODUCCION_LEXEMA || child->tipo == PRODUCCION_ETIQUETA);
	
	if (child->tipo == PRODUCCION_ETIQUETA) {
		TOKEN* token_etiqueta = seleccionar_token(produccion, "..");
		assert(token_etiqueta != NULL);
		
		SIMBOLO* variable = (*(programa->simbolos))[token_etiqueta->token];
		assert(variable != NULL);
		
		ch_generar_produccion(child, programa);
		
		PRODUCCION* corchete = seleccionar_produccion(child, ".>");
		if (corchete != NULL) {
			assert(variable->dim1 != 0);
			ch_insertar_instruccion(programa, CH_PUSHCI);
			ch_embedd_integer(programa, variable->dim1);
			ch_insertar_instruccion(programa, CH_POPZ);
			
			ch_insertar_instruccion(programa, CH_PUSHVM);
		} else {
			ch_insertar_instruccion(programa, CH_PUSHM);
		}
		ch_embedd_data_address(programa, child);
	} else {
		TOKEN* token_string = seleccionar_token(produccion, ".");
		ch_insertar_instruccion(programa, CH_PUSHCM);
		
		ch_embedd_string(programa, token_string);
	}
}
