#include <iostream>
#include <cstdlib>
#include <string>
#include <iomanip>
#include <assert.h>

using namespace std;

unsigned short direccion = 0;

typedef struct {
	string instruccion;
	void (*function) (FILE*);
} INSTRUCCION;

INSTRUCCION* crear_instruccion(string nombre, void (*function) (FILE*)) {
	INSTRUCCION* instruccion = new INSTRUCCION();
	instruccion->instruccion = nombre;
	instruccion->function = function;

	return instruccion;
}

void asm_etiqueta(FILE* entrada) {
	unsigned short etiqueta;
	fread(&etiqueta, sizeof(unsigned short), 1, entrada);
	cout << "V" << setw(4) << etiqueta;
	direccion += 2;
}

void asm_direccion_codigo(FILE* entrada) {
	unsigned short direccion_codigo;
	fread(&direccion_codigo, sizeof(unsigned short), 1, entrada);
	cout << "E" << setw(4) << direccion_codigo;
	direccion += 2;
}

void asm_constante_char(FILE* entrada) {
	char cchar;
	fread(&cchar, sizeof(char), 1, entrada);
	cout << "'" << cchar << "'";
	direccion += 1;
}

void asm_constante_int(FILE* entrada) {
	int cint;
	fread(&cint, sizeof(int), 1, entrada);
	cout << cint;
	direccion += 4;
}

void asm_constante_double(FILE* entrada) {
	double cdouble;
	fread(&cdouble, sizeof(double), 1, entrada);
	cout << cdouble;
	direccion += 8;
}

void asm_mensaje(FILE* entrada) {
	char tamanioc;
	fread(&tamanioc, sizeof(char), 1, entrada);
	
	int tamanio = tamanioc;
	direccion += 1;
	
	char* mensaje = new char[tamanio + 1];
	fread(mensaje, sizeof(char) * tamanio, 1, entrada);
	mensaje[tamanio] = 0;
	
	cout << "\"";
	direccion += tamanio;
	for (int i = 0; i < tamanio; i++) {
		if (mensaje[i] == 9) {
			cout << "\\t";
		} else if (mensaje[i] == 13) {
			if (i < tamanio - 1 && mensaje[i + 1] == 10) {
				cout << "\\n";
			} else {
				cout << "\\r";
			}
		} else if (mensaje[i] == 10) {
			if (i > 0 && mensaje[i - 1] != 13) {
				cout << "\\n";
			}
		} else if (mensaje[i] == '\a') {
			cout << "\\a";
		} else if (mensaje[i] == '\b') {
			cout << "\\b";
		} else if (mensaje[i] == '\f') {
			cout << "\\f";
		} else if (mensaje[i] == '\\') {
			cout << "\\";
		} else {
			cout << mensaje[i];
		}
	}
	cout << "\"";
	
	delete mensaje;
}

INSTRUCCION* instrucciones[63] = {
		crear_instruccion("HLT", NULL),
		crear_instruccion("ADD", NULL),
		crear_instruccion("SUB", NULL),
		crear_instruccion("MUL", NULL),
		crear_instruccion("DIV", NULL),
		crear_instruccion("MOD", NULL),
		crear_instruccion("INC", NULL),
		crear_instruccion("DEC", NULL),
		crear_instruccion("CMPEQ", NULL),
		crear_instruccion("CMPNE", NULL),
		crear_instruccion("CMPLT", NULL),
		crear_instruccion("CMPLE", NULL),
		crear_instruccion("CMPGT", NULL),
		crear_instruccion("CMPGE", NULL),
		crear_instruccion("NOT", NULL),
		crear_instruccion("AND", NULL),
		crear_instruccion("OR", NULL),
		crear_instruccion("JMP", asm_direccion_codigo),
		crear_instruccion("JMPT", asm_direccion_codigo),
		crear_instruccion("JMPF", asm_direccion_codigo),
		crear_instruccion("READC", asm_etiqueta),
		crear_instruccion("READI", asm_etiqueta),
		crear_instruccion("READD", asm_etiqueta),
		crear_instruccion("READVC", asm_etiqueta),
		crear_instruccion("READVI", asm_etiqueta),
		crear_instruccion("READVD", asm_etiqueta),
		crear_instruccion("READS", asm_etiqueta),
		crear_instruccion("READVS", asm_etiqueta),
		crear_instruccion("WRITEC", asm_etiqueta),
		crear_instruccion("WRITEI", asm_etiqueta),
		crear_instruccion("WRITED", asm_etiqueta),
		crear_instruccion("WRITEVC", asm_etiqueta),
		crear_instruccion("WRITEVI", asm_etiqueta),
		crear_instruccion("WRITEVD", asm_etiqueta),
		crear_instruccion("WRITECC", asm_constante_char),
		crear_instruccion("WRITECI", asm_constante_int),
		crear_instruccion("WRITECD", asm_constante_double),
		crear_instruccion("WRITES", asm_etiqueta),
		crear_instruccion("WRITEM", asm_mensaje),
		crear_instruccion("WRITEP", NULL),
		crear_instruccion("WRITECR", NULL),
		crear_instruccion("PUSHC", asm_etiqueta),
		crear_instruccion("PUSHI", asm_etiqueta),
		crear_instruccion("PUSHD", asm_etiqueta),
		crear_instruccion("PUSHVC", asm_etiqueta),
		crear_instruccion("PUSHVI", asm_etiqueta),
		crear_instruccion("PUSHVD", asm_etiqueta),
		crear_instruccion("PUSHVM", asm_etiqueta),
		crear_instruccion("PUSHM", asm_etiqueta),
		crear_instruccion("PUSHCC", asm_constante_char),
		crear_instruccion("PUSHCI", asm_constante_int),
		crear_instruccion("PUSHCD", asm_constante_double),
		crear_instruccion("PUSHCM", asm_mensaje),
		crear_instruccion("POPC", asm_etiqueta),
		crear_instruccion("POPI", asm_etiqueta),
		crear_instruccion("POPD", asm_etiqueta),
		crear_instruccion("POPVC", asm_etiqueta),
		crear_instruccion("POPVI", asm_etiqueta),
		crear_instruccion("POPVD", asm_etiqueta),
		crear_instruccion("POPVM", asm_etiqueta),
		crear_instruccion("POPM", asm_etiqueta),
		crear_instruccion("POPX", NULL),
		crear_instruccion("POPZ", NULL)
	};

int main(int argc, char *argv[]) {
	FILE* entrada;
	
	entrada = fopen(argv[1], "rb");

	if (entrada == NULL) {
		cout << "error, el archivo no existe :(" << endl;
		return EXIT_FAILURE;
	}
	
	cout << setfill('0');
	
	cout << ";CHD (c) 2007 Hilario Perez Corona" << endl;
	cout << ";V63.1" << endl;
	cout << endl;
	
	unsigned short dataSize;
	unsigned short codeSize;
	
	// leer el tamanio del segmento de datos
	fseek(entrada, 12, SEEK_SET);
	fread(&dataSize, sizeof(unsigned short), 1, entrada);
	fread(&codeSize, sizeof(unsigned short), 1, entrada);
	
	cout << "; data segment size: " << dataSize << endl;
	cout << "; code segment size: " << codeSize << endl;
	
	while (true) {
		char codigo;
		int count = fread(&codigo, sizeof(char), 1, entrada);
		if (count < 1) {
			break;
		}
		if (codigo < 0 || codigo > 62) {
			cerr << "[ERROR] codigo de operacion invalido: " << (int)codigo << endl;
			break;
		}
		
		int cod_operacion = (int)codigo;
		INSTRUCCION *instruccion = instrucciones[cod_operacion];
		assert(instruccion != NULL);
		
		cout << "E" << setw(4) << direccion << ":\t" << instruccion->instruccion;
		if (instruccion->function != NULL) {
			cout << "\t";
			instruccion->function(entrada);
		}
		
		cout << endl;
		direccion++;
	}

	fclose(entrada);

	return EXIT_SUCCESS;
}
