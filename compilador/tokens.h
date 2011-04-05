#ifndef TOKENS_H_
#define TOKENS_H_

#include <string>
#include <iostream>

using namespace std;

enum TIPO_TOKEN {
	TOKEN_INVALIDO,		// Token no reconocido
	TOKEN_ETIQUETA,		// Alguna etiqueta (variable posiblemente)
	TOKEN_LPAR,			// (
	TOKEN_RPAR,			// )
	TOKEN_LCOR,			// [
	TOKEN_RCOR,			// ]
	TOKEN_COMA,			// ,
	TOKEN_ASIGNA,		// =
	TOKEN_PRINT,		// print
	TOKEN_PRINTLN,		// println
	TOKEN_READ,			// read
	TOKEN_IF,			// if
	TOKEN_ELSE,			// else
	TOKEN_ENDIF,		// endif
	TOKEN_WHILE,		// while
	TOKEN_ENDWHILE,		// endwhile
	TOKEN_FOR,			// for
	TOKEN_ENDFOR,		// endfor
	TOKEN_INTEGER,		// integer
	TOKEN_DOUBLE,		// double
	TOKEN_CHAR,			// char
	TOKEN_START,		// start
	TOKEN_END,			// end
	TOKEN_STOP,			// stop
	TOKEN_ADD,			// +
	TOKEN_REST,			// -
	TOKEN_MUL,			// *
	TOKEN_DIV,			// /
	TOKEN_MOD,			// %
	TOKEN_CONCAT,		// &
	TOKEN_GT,			// >
	TOKEN_GE,			// >=
	TOKEN_EQ,			// ==
	TOKEN_LE,			// <=
	TOKEN_LT,			// <
	TOKEN_NE,			// !=
	TOKEN_AND,			// &&
	TOKEN_OR,			// ||
	TOKEN_STRING,		// String...
	TOKEN_NUMERO,		// Numero...
	TOKEN_ENTERO		// Numero Entero...
};

enum ESTADO_TOKENIZER {
	TOKENIZER_ESPACIO,
	TOKENIZER_NUMERO,
	TOKENIZER_TERMINO,
	TOKENIZER_CADENA,
	TOKENIZER_CARACTER,
	TOKENIZER_COMPARACION,
	TOKENIZER_COMENTARIO
};

typedef struct {
	string token;
	int columna;
	int renglon;
	TIPO_TOKEN tipo;
} TOKEN;


TOKEN* crear_token(string token, int renglon, int columna);
TIPO_TOKEN clasificar_token(string token);
bool expect_inicio_etiqueta(char c);
bool expect_caracter_etiqueta(char c);
void commit_token(string token, TOKEN** tokens, int &current, int renglon, int columna);
void tokenizar_archivo(FILE* entrada, TOKEN** tokens, int &current);
bool expect_digito(char c);
bool expect_digito_o_punto(char c);
bool analizador_lexico(TOKEN** tokens, int max);

#endif /*TOKENS_H_*/
