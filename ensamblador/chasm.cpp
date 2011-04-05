#include <iostream>
#include <cstdlib>
#include <string>
#include <map>
#include <list>

using namespace std;

struct strCmp {
	bool operator()(string s1, string s2) const {
		return s1 < s2;
	}
};

struct strCmpi {
	bool operator()(string s1, string s2) const {
		return s1 < s2;
	}
};

typedef struct {
	unsigned short direccion;
	string token;
} FALTANTE;

typedef struct {
	unsigned short codigo;
	bool (*function) (FILE*, FILE*, unsigned short);
} INSTRUCCION;

FALTANTE** faltantes = new FALTANTE*[100];
unsigned int faltantesCount = 0;

map<string, INSTRUCCION*, strCmpi> instrucciones_soportadas;

INSTRUCCION* crear_instruccion(unsigned short codigo, bool (*impl) (FILE*, FILE*, unsigned short)) {
	INSTRUCCION* instruccion = new INSTRUCCION();
	instruccion->codigo = codigo;
	instruccion->function = impl;

	return instruccion;
}

enum TIPO_DECLARACION {
	CHAR, ENTERO, DOBLE, ARREGLO_CHAR, ARREGLO_ENTERO, ARREGLO_DOBLE, ETIQUETA, ETIQUETA_FALTANTE
};

typedef struct {
	unsigned short direccion;
	TIPO_DECLARACION tipo;
} DECLARACION;

map<string, DECLARACION*, strCmp> declaraciones;

bool data_escrita = false;
unsigned short direccion = 0;
unsigned short data_segment = 0;
unsigned short code_segment = 0;

DECLARACION* crear_declaracion(TIPO_DECLARACION tipo) {
	DECLARACION* variable = new DECLARACION();
	variable->direccion = direccion;
	variable->tipo = tipo;

	return variable;
}


enum TIPO_TOKEN {
	TOKEN_INSTRUCCION, TOKEN_CONSTANTE, TOKEN_ETIQUETA, TOKEN_STRING, TOKEN_VARIABLE,
	TOKEN_FIN_DE_ARCHIVO, TOKEN_ERROR
};


char caracteres[4];
bool eof = false;
unsigned short linea = 1;
unsigned short columna = 0;
bool recuperar = false;

void crear_etiqueta_faltante(string token) {
	faltantes[faltantesCount] = new FALTANTE();
	faltantes[faltantesCount]->direccion = direccion;
	faltantes[faltantesCount]->token = token;
	faltantesCount++;
}

string leer_siguiente_token(FILE* entrada) {
	bool dentroString = false;
	bool dentroComentarios = false;
	bool dentroToken = false;
	bool dentroCaracter = false;
	unsigned short nCaracteres;
	char ultimoCaracter;
	char c;

	string token = "";

	while (true) {
		if (recuperar == false) {
			int leidos = fread(caracteres, sizeof(c), 1, entrada);
			if (leidos <= 0) {
				eof = true;
				return "";
			}
			columna++;
		}
		recuperar = false;

		c = caracteres[0];
		if (dentroComentarios) {
			if (c == 13 || c == 10) {
				// salto de linea
				if (c == 10) {
					linea++;
				}
				columna = 0;
				dentroComentarios = false;
			}
			continue;
		} else if (dentroString) {
			if (c == 13 || c == 10) {
				cout << "string no terminado" << endl;
				return "";
			}
			nCaracteres++;
			if (nCaracteres > 255) {
				cout << "el tamanio limite del string se excede" << endl;
				return "";
			}
			if (c == '"' && ultimoCaracter != '\\') {
				token = token + c;
				return token;
			}
		} else if (dentroCaracter) {
			if (c == 13 || c == 10) {
				cout << "caracter no terminado" << endl;
				return "";
			}
			nCaracteres++;
			if (nCaracteres > 2) {
				cout << "el tamanio limite del caracter se excede" << endl;
				return "";
			}
			if (c == '\'' && ultimoCaracter != '\\') {
				if (nCaracteres == 0) {
					cout << "no se especifico ningun caracter" << endl;
					return "";
				}
				caracteres[0] = 0;
				caracteres[1] = 0;
				caracteres[2] = 0;
				caracteres[3] = 0;
				sprintf(caracteres, "%d", (int)ultimoCaracter);
				token = caracteres;
				return token;
			}
		} else if (dentroToken) {
			if (c == ' ' || c == 9 || c == ',') {
				return token;
			} else if ((c >= 'a' && c <= 'z') ||
					(c >= 'A' && c <= 'Z') ||
					(c >= '0' && c <= '9') || c == '.' || c == ':' || c == '_') {
			} else if (c == ';') {
				recuperar = true;
				return token;
			} else if (c == 13 || c == 10) {
				// salto de linea
				if (c == 10) {
					linea++;
				}
				columna = 0;

				return token;
			} else {
				cout << "caracter invalido: " << c << endl;
				return "";
			}
		} else {
			if (c == ' ' || c == 9 || c == ',') {
				// omitir los espacios
				continue;
			} else if ((c >= 'a' && c <= 'z') ||
					(c >= 'A' && c <= 'Z') ||
					(c >= '0' && c <= '9') || 
					c == '.' || c == '-' || c == '_') {
				dentroToken = true;
			} else if (c == '\'') {
				dentroCaracter = true;
				nCaracteres = 0;
			} else if (c == '"') {
				dentroString = true;
				nCaracteres = 0;
			} else if (c == ';') {
				dentroComentarios = true;
				continue;
			} else if (c == 13 || c == 10) {
				// salto de linea
				if (c == 10) {
					linea++;
				}
				columna = 0;
				continue;
			} else {
				cout << "caracter invalido: " << c << endl;
				return "";
			}
		}

		token = token + c;
		ultimoCaracter = c;
	}

	return token;
}

bool es_numero(string token) {
	for (unsigned int i = 0; i < token.length(); i++) {
		char caracter = (char)token.at(i);

		if (caracter >= '0' && caracter <= '9') continue;
		if (caracter == '.') continue; //FIXME: le falta depuracion

		return false;
	}

	return true;
}

TIPO_TOKEN tipo_token(string token) {
	if (eof == true) {
		return TOKEN_FIN_DE_ARCHIVO;
	}

	if (token == "") {
		return TOKEN_ERROR;
	}

	INSTRUCCION* instruccion = instrucciones_soportadas[token];
	if (instruccion != NULL) {
		return TOKEN_INSTRUCCION;
	}

	DECLARACION* variable = declaraciones[token];
	if (variable != NULL) {
		if (variable->tipo == ETIQUETA) {
			return TOKEN_ETIQUETA;
		} else {
			return TOKEN_VARIABLE;
		}
	}

	if (token.substr(token.length() - 1) == ":") {
		return TOKEN_ETIQUETA;
	}

	if (token.at(0) == '"') {
		return TOKEN_STRING;
	}

	if (token.at(0) == '\'') {
		return TOKEN_CONSTANTE;
	}

	if (es_numero(token)) {
		return TOKEN_CONSTANTE;
	}

	return TOKEN_ETIQUETA;
}

DECLARACION* siguiente_token_direccion(FILE* entrada) {
	string token = leer_siguiente_token(entrada);

	TIPO_TOKEN tipo = tipo_token(token);
	if (tipo == TOKEN_ERROR || tipo == TOKEN_FIN_DE_ARCHIVO) {
		cout << "se esperaba una variable/etiqueta" << endl;
		return NULL;
	}

	DECLARACION* variable = declaraciones[token];
	if (variable == NULL) {
		cout << "identificador no encontrado: " << token << endl;
		return NULL;
	}

	return variable;
}

DECLARACION* siguiente_token_direccion_codigo(FILE* entrada) {
	string token = leer_siguiente_token(entrada);

	TIPO_TOKEN tipo = tipo_token(token);
	if (tipo == TOKEN_ERROR || tipo == TOKEN_FIN_DE_ARCHIVO) {
		cout << "se esperaba una variable/etiqueta" << endl;
		return NULL;
	}

	DECLARACION* variable = declaraciones[token];
	if (variable == NULL) {
		crear_etiqueta_faltante(token);
		variable = new DECLARACION();
		variable->tipo = ETIQUETA_FALTANTE;
	}

	return variable;
}

bool asm_direccion_codigo(FILE* entrada, FILE* salida, unsigned short inst) {
	DECLARACION* etiqueta = siguiente_token_direccion_codigo(entrada);

	if (etiqueta == NULL || (etiqueta->tipo != ETIQUETA && etiqueta->tipo != ETIQUETA_FALTANTE)) {
		cout << "se esperaba una etiqueta" << endl;
		return false;
	}

	fwrite(&(etiqueta->direccion), sizeof(short), 1, salida);

	direccion += 2;

	return true;
}

bool asm_direccion_datos(FILE* entrada, FILE* salida, unsigned short inst) {
	DECLARACION* variable = siguiente_token_direccion(entrada);

	if (variable == NULL || variable->tipo == ETIQUETA) {
		cout << "se esperaba una variable" << endl;
		return false;
	}

	fwrite(&variable->direccion, sizeof(short), 1, salida);

	direccion += 2;

	return true;
}

bool asm_constante_char(FILE* entrada, FILE* salida, unsigned short inst) {
	string token = leer_siguiente_token(entrada);

	if (tipo_token(token) != TOKEN_CONSTANTE) {
		cout << "se esperaba una constante" << endl;
		return false;
	}

	char c = (char)atoi(token.c_str());
	fwrite(&c, sizeof(char), 1, salida);

	direccion += 1;

	return true;
}

bool asm_constante_int(FILE* entrada, FILE* salida, unsigned short inst) {
	string token = leer_siguiente_token(entrada);

	if (tipo_token(token) != TOKEN_CONSTANTE) {
		cout << "se esperaba una constante" << endl;
		return false;
	}

	int c = (int)atoi(token.c_str());
	fwrite(&c, sizeof(int), 1, salida);

	direccion += 4;

	return true;
}

bool asm_constante_double(FILE* entrada, FILE* salida, unsigned short inst) {
	string token = leer_siguiente_token(entrada);

	if (tipo_token(token) != TOKEN_CONSTANTE) {
		cout << "se esperaba una constante" << endl;
		return false;
	}

	double c = (double)atof(token.c_str());

	fwrite(&c, sizeof(double), 1, salida);

	direccion += 8;

	return true;
}

bool asm_constante_string(FILE* entrada, FILE* salida, unsigned short inst) {
	string token = leer_siguiente_token(entrada);

	if (tipo_token(token) != TOKEN_STRING) {
		cout << "se esperaba un string" << endl;
		return false;
	}

	char* dir = (char*)token.c_str();
	dir++;

	char c = (char)token.length() - 2;
	
	fwrite(&c, sizeof(char), 1, salida);
	fwrite(dir, sizeof(char), token.length() - 2, salida);

	direccion += token.length() - 1;

	return true;
}

bool asm_declara_datos(FILE* entrada, FILE* salida, unsigned short inst) {
	string token = leer_siguiente_token(entrada);

	if (tipo_token(token) == TOKEN_VARIABLE) {
		cout << "no se puede duplicar el uso de un identificador: " << token << endl;
		return false;
	}

	if (tipo_token(token) != TOKEN_ETIQUETA) {
		cout << "se esperaba un identificador: " << token << endl;
		return false;
	}

	switch (inst) {
		case 501:
			declaraciones[token] = crear_declaracion(CHAR);
			direccion += 1;
			break;
		case 504:
			declaraciones[token] = crear_declaracion(ENTERO);
			direccion += 4;
			break;
		case 508:
			declaraciones[token] = crear_declaracion(DOBLE);
			direccion += 8;
			break;
		default:
			cout << "instruccion invalida" << endl;
	}
	
	return true;
}

bool asm_declara_datos_arreglo(FILE* entrada, FILE* salida, unsigned short inst) {
	string token = leer_siguiente_token(entrada);
	string constante = leer_siguiente_token(entrada);

	if (tipo_token(token) == TOKEN_VARIABLE) {
		cout << "no se puede duplicar el uso de un identificador: " << token << endl;
		return false;
	}

	if (tipo_token(token) != TOKEN_ETIQUETA) {
		cout << "se esperaba un identificador: " << token << endl;
		return false;
	}

	if (tipo_token(constante) != TOKEN_CONSTANTE) {
		cout << "no se especifico apropiadamente el tamanio del arreglo: " << constante << endl;
		return false;
	}

	int c = (int)atoi(constante.c_str());

	if (c < 0) {
		cout << "el tamanio del arreglo debe ser positivo: " << c << endl;
		return false;
	}

	switch (inst) {
		case 511:
			declaraciones[token] = crear_declaracion(ARREGLO_CHAR);
			direccion += 1 * c;
			break;
		case 514:
			declaraciones[token] = crear_declaracion(ARREGLO_ENTERO);
			direccion += 4 * c;
			break;
		case 518:
			declaraciones[token] = crear_declaracion(ARREGLO_DOBLE);
			direccion += 8 * c;
			break;
		default:
			cout << "instruccion invalida" << endl;
	}
	
	return true;
}

bool asm_declara_etiqueta_codigo(string token) {
	if (tipo_token(token) == TOKEN_VARIABLE || declaraciones[token] != NULL) {
		cout << "no se puede duplicar el uso de un identificador: " << token << endl;
		return false;
	}

	if (tipo_token(token) != TOKEN_ETIQUETA) {
		cout << "se esperaba un identificador: " << token << endl;
		return false;
	}

	declaraciones[token] = crear_declaracion(ETIQUETA);

	return true;
}

void inicializar_instrucciones() {
	instrucciones_soportadas["HLT"] = crear_instruccion(0, NULL);
	instrucciones_soportadas["ADD"] = crear_instruccion(1, NULL);
	instrucciones_soportadas["SUB"] = crear_instruccion(2, NULL);
	instrucciones_soportadas["MUL"] = crear_instruccion(3, NULL);
	instrucciones_soportadas["DIV"] = crear_instruccion(4, NULL);
	instrucciones_soportadas["MOD"] = crear_instruccion(5, NULL);
	instrucciones_soportadas["INC"] = crear_instruccion(6, NULL);
	instrucciones_soportadas["DEC"] = crear_instruccion(7, NULL);

	instrucciones_soportadas["CMPEQ"] = crear_instruccion(8, NULL);
	instrucciones_soportadas["CMPNE"] = crear_instruccion(9, NULL);
	instrucciones_soportadas["CMPLT"] = crear_instruccion(10, NULL);
	instrucciones_soportadas["CMPLE"] = crear_instruccion(11, NULL);
	instrucciones_soportadas["CMPGT"] = crear_instruccion(12, NULL);
	instrucciones_soportadas["CMPGE"] = crear_instruccion(13, NULL);

	instrucciones_soportadas["AND"] = crear_instruccion(14, NULL);
	instrucciones_soportadas["OR"] = crear_instruccion(15, NULL);

	instrucciones_soportadas["JMP"] = crear_instruccion(16, asm_direccion_codigo);
	instrucciones_soportadas["JMPT"] = crear_instruccion(17, asm_direccion_codigo);
	instrucciones_soportadas["JMPF"] = crear_instruccion(18, asm_direccion_codigo);

	instrucciones_soportadas["READC"] = crear_instruccion(19, asm_direccion_datos);
	instrucciones_soportadas["READI"] = crear_instruccion(20, asm_direccion_datos);
	instrucciones_soportadas["READD"] = crear_instruccion(21, asm_direccion_datos);
	instrucciones_soportadas["READVC"] = crear_instruccion(22, asm_direccion_datos);
	instrucciones_soportadas["READVI"] = crear_instruccion(23, asm_direccion_datos);
	instrucciones_soportadas["READVD"] = crear_instruccion(24, asm_direccion_datos);
	instrucciones_soportadas["READS"] = crear_instruccion(25, asm_direccion_datos);

	instrucciones_soportadas["WRITEC"] = crear_instruccion(26, asm_direccion_datos);
	instrucciones_soportadas["WRITEI"] = crear_instruccion(27, asm_direccion_datos);
	instrucciones_soportadas["WRITED"] = crear_instruccion(28, asm_direccion_datos);
	instrucciones_soportadas["WRITEVC"] = crear_instruccion(29, asm_direccion_datos);
	instrucciones_soportadas["WRITEVI"] = crear_instruccion(30, asm_direccion_datos);
	instrucciones_soportadas["WRITEVD"] = crear_instruccion(31, asm_direccion_datos);
	instrucciones_soportadas["WRITECC"] = crear_instruccion(32, asm_constante_char);
	instrucciones_soportadas["WRITECI"] = crear_instruccion(33, asm_constante_int);
	instrucciones_soportadas["WRITECD"] = crear_instruccion(34, asm_constante_double);
	instrucciones_soportadas["WRITES"] = crear_instruccion(35, asm_direccion_datos);
	instrucciones_soportadas["WRITEM"] = crear_instruccion(36, asm_constante_string);
	instrucciones_soportadas["WRITEP"] = crear_instruccion(37, NULL);
	instrucciones_soportadas["WRITECR"] = crear_instruccion(38, NULL);

	instrucciones_soportadas["PUSHC"] = crear_instruccion(39, asm_direccion_datos);
	instrucciones_soportadas["PUSHI"] = crear_instruccion(40, asm_direccion_datos);
	instrucciones_soportadas["PUSHD"] = crear_instruccion(41, asm_direccion_datos);
	instrucciones_soportadas["PUSHVC"] = crear_instruccion(42, asm_direccion_datos);
	instrucciones_soportadas["PUSHVI"] = crear_instruccion(43, asm_direccion_datos);
	instrucciones_soportadas["PUSHVD"] = crear_instruccion(44, asm_direccion_datos);
	instrucciones_soportadas["PUSHM"] = crear_instruccion(45, asm_direccion_datos);
	instrucciones_soportadas["PUSHCC"] = crear_instruccion(46, asm_constante_char);
	instrucciones_soportadas["PUSHCI"] = crear_instruccion(47, asm_constante_int);
	instrucciones_soportadas["PUSHCD"] = crear_instruccion(48, asm_constante_double);
	instrucciones_soportadas["PUSHCM"] = crear_instruccion(49, asm_constante_string);

	instrucciones_soportadas["POPC"] = crear_instruccion(50, asm_direccion_datos);
	instrucciones_soportadas["POPI"] = crear_instruccion(51, asm_direccion_datos);
	instrucciones_soportadas["POPD"] = crear_instruccion(52, asm_direccion_datos);
	instrucciones_soportadas["POPVC"] = crear_instruccion(53, asm_direccion_datos);
	instrucciones_soportadas["POPVI"] = crear_instruccion(54, asm_direccion_datos);
	instrucciones_soportadas["POPVD"] = crear_instruccion(55, asm_direccion_datos);
	instrucciones_soportadas["POPM"] = crear_instruccion(56, asm_direccion_datos);
	instrucciones_soportadas["POPX"] = crear_instruccion(57, NULL);

	// DEFINICION DE VARIABLES (no generan codigo)
	instrucciones_soportadas["DEFC"] = crear_instruccion(501, asm_declara_datos);
	instrucciones_soportadas["DEFI"] = crear_instruccion(504, asm_declara_datos);
	instrucciones_soportadas["DEFD"] = crear_instruccion(508, asm_declara_datos);
	instrucciones_soportadas["DEFVC"] = crear_instruccion(511, asm_declara_datos_arreglo);
	instrucciones_soportadas["DEFVI"] = crear_instruccion(514, asm_declara_datos_arreglo);
	instrucciones_soportadas["DEFVD"] = crear_instruccion(518, asm_declara_datos_arreglo);
}

bool procesar(FILE* entrada, FILE* salida, string token, TIPO_TOKEN tipo) {
	if (tipo == TOKEN_ETIQUETA) {
		if (token.substr(token.length() - 1) != ":") {
			cout << "error, token invalido: " << token << endl;
			return false;
		}
		token = token.substr(0, token.length() - 1);

		if (asm_declara_etiqueta_codigo(token) == false) {
			cout << "error, al declarar la etiqueta: " << token << endl;
			return false;
		}
	} else if (tipo == TOKEN_INSTRUCCION) {
		INSTRUCCION* instruccion = instrucciones_soportadas[token];

		if (instruccion->codigo <= 255) {
			if (data_escrita == false) {
				data_segment = direccion;

				fwrite(&data_segment, sizeof(short), 1, salida);
				fwrite(&data_segment, sizeof(short), 1, salida);

				// segmentos de dato y codigo seran diferentes
				direccion = 0;

				data_escrita = true;
			}
			// declaraciones que generan codigo
			char codigo = (char)instruccion->codigo;
			fwrite(&codigo, sizeof(codigo), 1, salida);

			direccion++;
		}
		if (instruccion->function != NULL) {
			if (instruccion->function(entrada, salida, instruccion->codigo) == false) {
				cout << "error, al evaluar los parametros del token: " << token << endl;
				return false;
			}
		}
	} else if (tipo == TOKEN_ERROR) {
		cout << "error, se detecto un token invalido" << endl;
		return false;
	} else {
		cout << "error, token invalido: " << token << endl;
		return false;
	}

	return true;
}

bool ensamblar(FILE* entrada, FILE* salida) {
	string token;
	TIPO_TOKEN tipo;

	while (true) {
		token = leer_siguiente_token(entrada);
		tipo = tipo_token(token);
		if (tipo == TOKEN_FIN_DE_ARCHIVO) break;

		if (tipo == TOKEN_ERROR) {
			return false;
		}

		if (tipo != TOKEN_INSTRUCCION && tipo != TOKEN_ETIQUETA) {
			cout << "error, instruccion invalida: " << token << endl;
			return false;
		}

		try {
			if (procesar(entrada, salida, token, tipo) == false) {
				cout << "error, no se pudo terminar de ensamblar" << endl;
				return false;
			}
		} catch (const char* e) {
			cout << e << endl;
			return false;
		}
	}
	return true;
}

bool establecer_etiquetas_faltantes(FILE* salida) {
	for (unsigned int i = 0; i < faltantesCount; i++) {
		DECLARACION* declaracion = declaraciones[faltantes[i]->token];
		if (declaracion == NULL || declaracion->tipo != ETIQUETA) {
			cout << "identificador no encontrado: " << faltantes[i]->token << endl;
			return false;
		}

		fseek(salida, 16 + faltantes[i]->direccion, SEEK_SET);
		fwrite(&declaracion->direccion, sizeof(short), 1, salida);

		delete faltantes[i];
	}

	delete []faltantes;

	return true;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		cout << "Modo de uso:" << endl;
		cout << "    chasm <archivo>" << endl;
		return EXIT_FAILURE;
	}

	FILE* entrada;

	cout << "ensamblando " << argv[1] << "..." << endl;

	inicializar_instrucciones();

	entrada = fopen(argv[1], "r");

	if (entrada == NULL) {
		cout << "error, el archivo no existe :(" << endl;
		return EXIT_FAILURE;
	}

	string nombre = argv[1];
	nombre = nombre.substr(0, nombre.find_last_of(".")) + ".cch";
	cout << "el archivo ensamblado sera " << nombre << endl;

	FILE* salida;

	salida = fopen(nombre.c_str(), "w+b");

	fprintf(salida, "(C)ChAsmICC1");

	if (salida == NULL) {
		cout << "error, no se pudo crear el archivo de salida" << endl;
		fclose(entrada);
		return EXIT_FAILURE;
	}

	if (ensamblar(entrada, salida) == false) {
		cout << "[CHASM] error, linea " << linea << " columna " << columna << endl;
		cout << "error, el ensamblado no pudo ser completado... vea los errores" << endl;
	} else {
		code_segment = direccion;

		fseek(salida, 14, SEEK_SET);
		fwrite(&code_segment, sizeof(short), 1, salida);

		if (establecer_etiquetas_faltantes(salida) == false) {
			cout << "error, el ensamblado no pudo ser completado... vea los errores" << endl;
		} else {
			cout << "ensamblado completado" << endl;
		}
	}

	fclose(salida);
	fclose(entrada);

	return EXIT_SUCCESS;
}

