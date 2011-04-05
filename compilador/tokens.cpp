#include "tokens.h"

bool analizador_lexico(TOKEN** tokens, int max) {
	bool todoBien = true;
	for (int i = 0; i < max; i++) {
		if (tokens[i]->tipo == TOKEN_INVALIDO) {
			cout << "[LEXICO] error, token invalido: " << tokens[i]->token << endl;
			cout << "         en el renglon " << tokens[i]->renglon << " columna " << tokens[i]->columna << endl;
			todoBien = false;
		}
	}
	
	return todoBien;
}

void tokenizar_archivo(FILE* entrada, TOKEN** tokens, int &current) {
	char c[2];
	int leidos = 1, renglon = 1, columna = 0;
	bool retake = false;
	bool diagonal = false;
	char anterior = 0;
	string token = "";
	ESTADO_TOKENIZER estado = TOKENIZER_ESPACIO;
	c[1] = 0;
	
	while (leidos > 0) {
		if (retake == false) {
			leidos = fread(c, sizeof(char), 1, entrada);
			anterior = c[0];
			columna++;
			
			if (c[0] == '\t') {
				columna += 3;
			}
		} else {
			retake = false;
			c[0] = anterior;
		}
		
		if (leidos > 0) {
			switch (estado) {
			case TOKENIZER_ESPACIO:
				token.clear();
				if ((c[0] >= 'a' && c[0] <= 'z') ||
						(c[0] >= 'A' && c[0] <= 'Z') ||
						c[0] == ':' || c[0] == '$' || c[0] == '_') {
					token = c;
					estado = TOKENIZER_TERMINO;
				} else if ((c[0] >= '0' && c[0] <= '9') || c[0] == '.') {
					token = c;
					estado = TOKENIZER_NUMERO;
				} else if (c[0] == '(' || c[0] == ')' || c[0] == '[' || c[0] == ']' ||
						c[0] == '+' || c[0] == '-' || c[0] == '*' || c[0] == '/' ||
						c[0] == '%') {
					token = c;
					commit_token(token, tokens, current, renglon, columna);
					token.clear();
				} else if (c[0] == '>' || c[0] == '=' || c[0] == '<' || c[0] == '&' ||
						c[0] == '|' || c[0] == '!') {
					token = c;
					estado = TOKENIZER_COMPARACION;
				} else if (c[0] == '"') {
					token = c;
					estado = TOKENIZER_CADENA;
					diagonal = false;
				} else if (c[0] == '\'') {
					token = c;
					estado = TOKENIZER_CARACTER;
				} else if (c[0] == ';') {
					estado = TOKENIZER_COMENTARIO; 
				} else if (c[0] == 10) {
					renglon++;
					columna = 0;
				} else if (c[0] != ' ' && c[0] != '\t' && c[0] != 13) {
					// caracter invalido
					token = c;
					commit_token(token, tokens, current, renglon, columna);
					token.clear();
				}
				break;
			case TOKENIZER_NUMERO:
				if ((c[0] >= '0' && c[0] <= '9') || c[0] == '.') {
					token += c;
				} else {
					commit_token(token, tokens, current, renglon, columna);
					token.clear();
					retake = true;
					estado = TOKENIZER_ESPACIO;
				}
				break;
			case TOKENIZER_TERMINO:
				if ((c[0] >= 'a' && c[0] <= 'z') ||
						(c[0] >= 'A' && c[0] <= 'Z') ||
						c[0] == ':' || c[0] == '$' || c[0] == '_' ||
						(c[0] >= '0' && c[0] <= '9')) {
					token += c;
				} else {
					commit_token(token, tokens, current, renglon, columna);
					token.clear();
					retake = true;
					estado = TOKENIZER_ESPACIO;
				}
				break;
			case TOKENIZER_CADENA:
				if (c[0] == '\\' && diagonal == false) {
					diagonal = true;
				} else if (c[0] == '"' && diagonal == true) {
					diagonal = false;
					token += c;
				} else if (c[0] == '"' && diagonal == false) {
					token += c;
					commit_token(token, tokens, current, renglon, columna);
					token.clear();
					estado = TOKENIZER_ESPACIO;
				} else if (c[0] == 13 || c[0] == 10) {
					commit_token(token, tokens, current, renglon, columna);
					token.clear();
					retake = true;
					estado = TOKENIZER_ESPACIO;
				} else {
					if (diagonal == true) {
						switch(c[0]) {
						case 't':
							c[0] = '\t';
							break;
						case 'n':
							c[0] = '\n';
							break;
						case 'r':
							c[0] = '\r';
							break;
						case 'a':
							c[0] = '\a';
							break;
						case 'b':
							c[0] = '\b';
							break;
						case 'f':
							c[0] = '\f';
							break;
						case '\\':
							c[0] = '\\';
							break;
						}
					}
					diagonal = false;
					token += c;
				}
				break;
			case TOKENIZER_CARACTER:
				if (c[0] == 13 || c[0] == 10) {
					commit_token(token, tokens, current, renglon, columna);
					token.clear();
					estado = TOKENIZER_ESPACIO;
				} else if (c[0] == '\'') {
					if (token.length() == 2) {
						token = token.substr(1, 1);
						int valorEntero = atoi(token.c_str());
						token = "" + valorEntero;
						
						commit_token(token, tokens, current, renglon, columna);
						token.clear();
						estado = TOKENIZER_ESPACIO;
					} else {
						commit_token(token, tokens, current, renglon, columna);
						token.clear();
						estado = TOKENIZER_ESPACIO;
					}
				}
				break;
			case TOKENIZER_COMPARACION:
				if (c[0] == '&' || c[0] == '|' || c[0] == '=') {
					token += c;
				} else {
					commit_token(token, tokens, current, renglon, columna);
					token.clear();
					retake = true;
					estado = TOKENIZER_ESPACIO;
				}
				break;
			case TOKENIZER_COMENTARIO:
				if (c[0] == 10) {
					token.clear();
					retake = true;
					estado = TOKENIZER_ESPACIO;
				}
			default:
				break;
			}
			
		} else {
			commit_token(token, tokens, current, renglon, columna);
		}
	}
}

void commit_token(string token, TOKEN** tokens, int &current, int renglon, int columna) {
	if (token.length() > 0) {
		TOKEN* obj_token = crear_token(token, renglon, columna - token.length());
		tokens[current++] = obj_token;
	}
}

TOKEN* crear_token(string token, int renglon, int columna) {
	TOKEN* obj_token = new TOKEN();
	
	obj_token->token = token;
	obj_token->renglon = renglon;
	obj_token->columna = columna;
	
	obj_token->tipo = clasificar_token(token);
	
	return obj_token;
}

TIPO_TOKEN clasificar_token(string token) {
	if (token == "(") {
		return TOKEN_LPAR;
	} else if (token == ")") {
		return TOKEN_RPAR;
	} else if (token == "[") {
		return TOKEN_LCOR;
	} else if (token == "]") {
		return TOKEN_RCOR;
	} else if (token == ",") {
		return TOKEN_COMA;
	} else if (token == "=") {
		return TOKEN_ASIGNA;
	} else if (token == "print") {
		return TOKEN_PRINT;
	} else if (token == "println") {
		return TOKEN_PRINTLN;
	} else if (token == "read") {
		return TOKEN_READ;
	} else if (token == "if") {
		return TOKEN_IF;
	} else if (token == "else") {
		return TOKEN_ELSE;
	} else if (token == "endif") {
		return TOKEN_ENDIF;
	} else if (token == "while") {
		return TOKEN_WHILE;
	} else if (token == "endwhile") {
		return TOKEN_ENDWHILE;
	} else if (token == "for") {
		return TOKEN_FOR;
	} else if (token == "endfor") {
		return TOKEN_ENDFOR;
	} else if (token == "integer") {
		return TOKEN_INTEGER;
	} else if (token == "double") {
		return TOKEN_DOUBLE;
	} else if (token == "char") {
		return TOKEN_CHAR;
	} else if (token == "start") {
		return TOKEN_START;
	} else if (token == "stop") {
		return TOKEN_STOP;
	} else if (token == "end") {
		return TOKEN_END;
	} else if (token == "+") {
		return TOKEN_ADD;
	} else if (token == "-") {
		return TOKEN_REST;
	} else if (token == "*") {
		return TOKEN_MUL;
	} else if (token == "/") {
		return TOKEN_DIV;
	} else if (token == "%") {
		return TOKEN_MOD;
	} else if (token == "&") {
		return TOKEN_CONCAT;
	} else if (token == ">") {
		return TOKEN_GT;
	} else if (token == ">=") {
		return TOKEN_GE;
	} else if (token == "==") {
		return TOKEN_EQ;
	} else if (token == "<=") {
		return TOKEN_LE;
	} else if (token == "<") {
		return TOKEN_LT;
	} else if (token == "!=") {
		return TOKEN_NE;
	} else if (token == "&&") {
		return TOKEN_AND;
	} else if (token == "||") {
		return TOKEN_OR;
	} else {
		char* data = (char*)token.c_str();
		if (token.length() > 0 && expect_inicio_etiqueta(data[0]) == true) {
			bool etiqueta = true;
			for (unsigned int i = 1; i < token.length(); i++) {
				if (expect_caracter_etiqueta(data[i]) == false) {
					etiqueta = false;
					break;
				}
			}
			
			if (etiqueta == true) {
				return TOKEN_ETIQUETA;
			}
		}
		
		if (token.length() > 0) {
			bool punto = false;
			bool numero = true;
			for (unsigned int i = 0; i < token.length(); i++) {
				if (punto == false) {
					if (expect_digito(data[i]) == false) {
						numero = false;
						break;
					}
				} else {
					if (expect_digito(data[i]) == false) {
						numero = false;
						break;
					} else {
						if (data[i] == '.') {
							punto = true;
						}
					}
				}
			}
			
			if (numero == true) {
				if (punto == false) {
					return TOKEN_ENTERO;
				} else {
					return TOKEN_NUMERO;
				}
			}
		}
		
		if (token.length() > 1 && data[0] == '\"' && data[token.length() - 1] == '\"') {
			bool esString = true;
			for (unsigned int i = 1; i < token.length() - 1; i++) {
				if (data[i] == '\"' && data[i - 1] != '\\') {
					esString = false;
					break;
				}
			}
			
			if (esString) {
				return TOKEN_STRING;
			}
		}
	}
	
	return TOKEN_INVALIDO;
}

bool expect_inicio_etiqueta(char c) {
	return (c >= 'a' && c <= 'z') 
			|| (c >= 'A' && c <= 'Z') 
			|| c == ':' || c == '_' || c == '$';
}

bool expect_caracter_etiqueta(char c) {
	return expect_inicio_etiqueta(c) || (c >= '0' && c <= '9');
}

bool expect_digito(char c) {
	return (c >= '0' && c <= '9');
}

bool expect_digito_o_punto(char c) {
	return (c >= '0' && c <= '9') || c == '.';
}
