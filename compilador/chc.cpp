#include <iostream>
#include <cstdlib>
#include <string>
#include <map>

#include "tokens.h"
#include "parser.h"
#include "types.h"
#include "gc_chvm.h"
#include "gc_llvm.h"

using namespace std;

CONTEXTO contexto;

TABLA_SIMBOLOS *tabla_simbolos;

int main(int argc, char *argv[]) {
	bool LLVM = false;

	if (argc != 2 && argc != 3) {
		cout << "Modo de uso:" << endl;
		cout << "    chc <archivo> [LLVM]" << endl;
		return EXIT_FAILURE;
	}

	if (argc > 2 && strcmp(argv[2], "LLVM") == 0) {
		LLVM = true;
	}
	
	FILE* entrada;
	
	entrada = fopen(argv[1], "r");
	if (entrada == NULL) {
		cout << "error, el archivo " << argv[1] << " no pudo ser abierto" << endl;
		
		return EXIT_FAILURE;
	}
	
	string nombre = argv[1];
	if (LLVM) {
		nombre = nombre.substr(0, nombre.find_last_of("."));
	} else {
		nombre = nombre.substr(0, nombre.find_last_of(".")) + ".cch";
	}

	tokenizar_archivo(entrada, contexto.tokens, contexto.total);
	fclose(entrada);
	
	if (analizador_lexico(contexto.tokens, contexto.total) == false) {
		cerr << "[ERROR] no se puede compilar debido a que se encontraron errores" << endl;
		
		return EXIT_FAILURE;
	} else {
		PRODUCCION *ch;
		try {
			ch = produccion_ch(&contexto);
			
			tabla_simbolos = verificar_tipos(ch);
			
			if (argc > 2 && strcmp(argv[2], "DEBUG") == 0) {
				debug_produccion(ch);
			}

			if (LLVM) {
				generar_codigo_llvm(ch, tabla_simbolos, nombre);
			} else {
				generar_codigo_chvm(ch, tabla_simbolos, nombre);
			}
			
			remover_produccion(ch);
		} catch (char *error) {
			cerr << error;
			
			cerr << "[ERROR] no se puede compilar debido a que se encontraron errores" << endl;
			return EXIT_FAILURE;
		}
	}
	
	return EXIT_SUCCESS;
}
