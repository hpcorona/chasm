#include <iostream>
#include <cstdlib>
#include <string>

#define 	MAX_STACK	100

unsigned short dataSegmentSize;
unsigned short codeSegmentSize;

using namespace std;

enum TIPO_DATO {
	CLEAR, CARACTER, ENTERO, DOBLE, STRING
};

typedef struct {
	string instruccion;
	bool (*function) ();
} INSTRUCCION;

typedef struct {
	union {
		char datoChar;
		int datoInt;
		double datoDouble;
		string *datoString;
	} dato;
	TIPO_DATO tipo;
} STACK;

FILE* entrada;

char* code_segment;
char* data_segment;
STACK* stack;

unsigned short stackPointer;
unsigned short programCounter;
int compareResult;
unsigned short idx;
unsigned short size;
bool HALT = false;

bool DEBUG = false;

INSTRUCCION* crear_instruccion(string nombre, bool (*function) ()) {
	INSTRUCCION* instruccion = new INSTRUCCION();
	instruccion->instruccion = nombre;
	instruccion->function = function;

	return instruccion;
}

void sysmsg(string msg) {
	if (DEBUG) {
		cout << "\t\t\t[CH_VM] " << msg << endl;
	}
}

void sysmsg(long val) {
	if (DEBUG) {
		cout << "\t\t\t[CH_VM] " << val << endl;
	}
}

bool ajustar_stack(int idx1, int idx2) {
	if (stack[idx1].tipo != stack[idx2].tipo) {
		switch(stack[idx1].tipo) {
			case CARACTER:
				if (stack[idx2].tipo == ENTERO) {
					stack[idx2].dato.datoChar = (char)stack[idx2].dato.datoInt;
				} else if (stack[idx2].tipo == DOBLE) {
					stack[idx2].dato.datoChar = (char)stack[idx2].dato.datoDouble;
				} else if (stack[idx2].tipo == STRING) {
					cout << "\n[RUNTIME ERROR] operacion invalida con un string (ajustar_stack)" << endl;
					return false;
				}
				break;
			case ENTERO:
				if (stack[idx2].tipo == CARACTER) {
					stack[idx2].dato.datoInt = (int)stack[idx2].dato.datoChar;
				} else if (stack[idx2].tipo == DOBLE) {
					stack[idx2].dato.datoInt = (int)stack[idx2].dato.datoDouble;
				} else if (stack[idx2].tipo == STRING) {
					cout << "\n[RUNTIME ERROR] operacion invalida con un string (ajustar_stack)" << endl;
					return false;
				}
				break;
			case DOBLE:
				if (stack[idx2].tipo == ENTERO) {
					stack[idx2].dato.datoDouble = (double)stack[idx2].dato.datoInt;
				} else if (stack[idx2].tipo == CARACTER) {
					stack[idx2].dato.datoDouble = (double)stack[idx2].dato.datoChar;
				} else if (stack[idx2].tipo == STRING) {
					cout << "\n[RUNTIME ERROR] operacion invalida con un string (ajustar_stack)" << endl;
					return false;
				}
				break;
			case STRING:
				char datos[100];
				if (stack[idx2].tipo == ENTERO) {
					sprintf(datos, "%d", stack[idx2].dato.datoInt);
				} else if (stack[idx2].tipo == CARACTER) {
					datos[0] = stack[idx2].dato.datoChar;
					datos[1] = 0;
				} else if (stack[idx2].tipo == DOBLE) {
					sprintf(datos, "%f", stack[idx2].dato.datoDouble);
				} else {
					break;
				}
				stack[idx2].dato.datoString = new string(datos);
				break;
			case CLEAR:
				cout << "\n[RUNTIME ERROR] valor nulo en el stack (ajustar_stack)" << endl;
				return false;
		}
		stack[idx2].tipo = stack[idx1].tipo;
	}
	
	return true;
}

bool ajustar_stack() {
	int idx1 = stackPointer - 1;
	int idx2 = stackPointer - 2;

	if (idx1 < 0 || idx2 < 0) {
		sysmsg("indice de stack invalido");
		sysmsg(idx1);
		sysmsg(idx2);
		return false;
	}

	if (stack[idx1].tipo != stack[idx2].tipo) {
		if (stack[idx1].tipo > stack[idx2].tipo) {
			if (ajustar_stack(idx1, idx2) == false) {
				return false;
			}
		} else {
			if (ajustar_stack(idx2, idx1) == false) {
				return false;
			}
		}
	}

	return true;
}

STACK* pop_stack() {
	STACK* ultimo = &stack[--stackPointer];

	if (DEBUG == true) {
		switch(ultimo->tipo) {
			case CARACTER:
				cout << "\t" << ultimo->dato.datoChar;
				break;
			case ENTERO:
				cout << "\t" << ultimo->dato.datoInt;
				break;
			case DOBLE:
				cout << "\t" << ultimo->dato.datoDouble;
				break;
			case STRING:
				cout << "\t\"" << *ultimo->dato.datoString << "\"";
				break;
			case CLEAR:
				cout << "\n[RUNTIME ERROR] valor nulo en el stack (pop_stack)" << endl;
				return NULL;
		}
	}

	return ultimo;
}

STACK* peek_stack() {
	return &stack[stackPointer - 1];
}

void delete_string() {
	if (stack[stackPointer].tipo == STRING) {
		// si habia un String, eliminarlo primero :)
		delete stack[stackPointer].dato.datoString;
		stack[stackPointer].tipo = CLEAR;
	}
}

void push_stack(char v) {
	delete_string();

	stack[stackPointer].tipo = CARACTER;
	stack[stackPointer].dato.datoChar = v;

	if (DEBUG == true) {
		cout << "\t" << v;
	}

	stackPointer++;
}

void push_stack(int v) {
	delete_string();
	
	stack[stackPointer].tipo = ENTERO;
	stack[stackPointer].dato.datoInt = v;

	if (DEBUG == true) {
		cout << "\t" << v;
	}

	stackPointer++;
}

void push_stack(double v) {
	delete_string();
	
	stack[stackPointer].tipo = DOBLE;
	stack[stackPointer].dato.datoDouble = v;

	if (DEBUG == true) {
		cout << "\t" << v;
	}

	stackPointer++;
}

void push_stack(string v) {
	stack[stackPointer].tipo = STRING;
	stack[stackPointer].dato.datoString = new string(v);

	if (DEBUG == true) {
		cout << "\t\"" << v << "\"";
	}

	stackPointer++;
}

unsigned short leer_direccion_codigo() {
	unsigned short* direccion;

	direccion = (unsigned short*)(code_segment + programCounter);

	programCounter += 2;

	return direccion[0];
}

char leer_caracter_codigo() {
	char* direccion;

	direccion = (char*)(code_segment + programCounter);

	programCounter += 1;

	return direccion[0];
}

int leer_entero_codigo() {
	int* direccion;

	direccion = (int*)(code_segment + programCounter);

	programCounter += 4;

	return direccion[0];
}

double leer_doble_codigo() {
	double* direccion;

	direccion = (double*)(code_segment + programCounter);

	programCounter += 8;

	return direccion[0];
}

bool asm_hlt() {
	HALT = true;
	return true;
}

bool asm_add() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			v1->dato.datoChar += v2->dato.datoChar;
			push_stack(v1->dato.datoChar);
			break;
		case ENTERO:
			v1->dato.datoInt += v2->dato.datoInt;
			push_stack(v1->dato.datoInt);
			break;
		case DOBLE:
			v1->dato.datoDouble += v2->dato.datoDouble;
			push_stack(v1->dato.datoDouble);
			break;
		case STRING:
			*v1->dato.datoString = *v1->dato.datoString + *v2->dato.datoString;
			push_stack(*v1->dato.datoString);
			break;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_add)" << endl;
			return false;
	}

	return true;
}

bool asm_sub() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			v1->dato.datoChar -= v2->dato.datoChar;
			push_stack(v1->dato.datoChar);
			break;
		case ENTERO:
			v1->dato.datoInt -= v2->dato.datoInt;
			push_stack(v1->dato.datoInt);
			break;
		case DOBLE:
			v1->dato.datoDouble -= v2->dato.datoDouble;
			push_stack(v1->dato.datoDouble);
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_sub)" << endl;
			return false;
	}

	return true;
}

bool asm_mul() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			v1->dato.datoChar *= v2->dato.datoChar;
			push_stack(v1->dato.datoChar);
			break;
		case ENTERO:
			v1->dato.datoInt *= v2->dato.datoInt;
			push_stack(v1->dato.datoInt);
			break;
		case DOBLE:
			v1->dato.datoDouble *= v2->dato.datoDouble;
			push_stack(v1->dato.datoDouble);
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_mul)" << endl;
			return false;
	}

	return true;
}

bool asm_div() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			v1->dato.datoChar /= v2->dato.datoChar;
			push_stack(v1->dato.datoChar);
			break;
		case ENTERO:
			v1->dato.datoInt /= v2->dato.datoInt;
			push_stack(v1->dato.datoInt);
			break;
		case DOBLE:
			v1->dato.datoDouble /= v2->dato.datoDouble;
			push_stack(v1->dato.datoDouble);
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_div)" << endl;
			return false;
	}

	return true;
}

bool asm_mod() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			v1->dato.datoChar = v1->dato.datoChar % v2->dato.datoChar;
			push_stack(v1->dato.datoChar);
			break;
		case ENTERO:
			v1->dato.datoInt = v1->dato.datoInt % v2->dato.datoInt;
			push_stack(v1->dato.datoInt);
			break;
		case DOBLE:
			v1->dato.datoDouble = (double)( ((long)v1->dato.datoDouble * 1000) %  ((long)v2->dato.datoDouble * 1000) ) / (double)1000;
			push_stack(v1->dato.datoDouble);
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_mod)" << endl;
			return false;
	}

	return true;
}

bool asm_inc() {
	STACK* v1 = peek_stack();

	switch(v1->tipo) {
		case CARACTER:
			v1->dato.datoChar += 1;
			break;
		case ENTERO:
			v1->dato.datoInt += 1;
			break;
		case DOBLE:
			v1->dato.datoDouble += 1;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_inc)" << endl;
			return false;
	}

	return true;
}

bool asm_dec() {
	STACK* v1 = peek_stack();

	switch(v1->tipo) {
		case CARACTER:
			v1->dato.datoChar -= 1;
			break;
		case ENTERO:
			v1->dato.datoInt -= 1;
			break;
		case DOBLE:
			v1->dato.datoDouble -= 1;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_dec)" << endl;
			return false;
	}

	return true;
}

bool asm_cmpeq() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			compareResult = v1->dato.datoChar == v2->dato.datoChar;
			break;
		case ENTERO:
			compareResult = v1->dato.datoInt == v2->dato.datoInt;
			break;
		case DOBLE:
			compareResult = v1->dato.datoDouble == v2->dato.datoDouble;
			break;
		case STRING:
			compareResult = *v1->dato.datoString == *v2->dato.datoString;
			break;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_cmpeq)" << endl;
			return false;
	}

	push_stack(compareResult);

	return true;
}

bool asm_cmpne() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			compareResult = v1->dato.datoChar != v2->dato.datoChar;
			break;
		case ENTERO:
			compareResult = v1->dato.datoInt != v2->dato.datoInt;
			break;
		case DOBLE:
			compareResult = v1->dato.datoDouble != v2->dato.datoDouble;
			break;
		case STRING:
			compareResult = *v1->dato.datoString != *v2->dato.datoString;
			break;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_cmpne)" << endl;
			return false;
	}

	push_stack(compareResult);

	return true;
}

bool asm_cmplt() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			compareResult = v1->dato.datoChar < v2->dato.datoChar;
			break;
		case ENTERO:
			compareResult = v1->dato.datoInt < v2->dato.datoInt;
			break;
		case DOBLE:
			compareResult = v1->dato.datoDouble < v2->dato.datoDouble;
			break;
		case STRING:
			compareResult = *v1->dato.datoString < *v2->dato.datoString;
			break;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_cmplt)" << endl;
			return false;
	}

	push_stack(compareResult);

	return true;
}

bool asm_cmple() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			compareResult = v1->dato.datoChar <= v2->dato.datoChar;
			break;
		case ENTERO:
			compareResult = v1->dato.datoInt <= v2->dato.datoInt;
			break;
		case DOBLE:
			compareResult = v1->dato.datoDouble <= v2->dato.datoDouble;
			break;
		case STRING:
			compareResult = *v1->dato.datoString <= *v2->dato.datoString;
			break;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_cmple)" << endl;
			return false;
	}

	push_stack(compareResult);

	return true;
}

bool asm_cmpgt() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			compareResult = v1->dato.datoChar > v2->dato.datoChar;
			break;
		case ENTERO:
			compareResult = v1->dato.datoInt > v2->dato.datoInt;
			break;
		case DOBLE:
			compareResult = v1->dato.datoDouble > v2->dato.datoDouble;
			break;
		case STRING:
			compareResult = *v1->dato.datoString > *v2->dato.datoString;
			break;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_cmpgt)" << endl;
			return false;
	}

	push_stack(compareResult);

	return true;
}

bool asm_cmpge() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			compareResult = v1->dato.datoChar >= v2->dato.datoChar;
			break;
		case ENTERO:
			compareResult = v1->dato.datoInt >= v2->dato.datoInt;
			break;
		case DOBLE:
			compareResult = v1->dato.datoDouble >= v2->dato.datoDouble;
			break;
		case STRING:
			compareResult = *v1->dato.datoString >= *v2->dato.datoString;
			break;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_cmpge)" << endl;
			return false;
	}

	push_stack(compareResult);

	return true;
}

bool asm_not() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v1 = pop_stack();
	bool valor = true;

	switch(v1->tipo) {
		case CARACTER:
			if (v1->dato.datoChar != 0) {
				valor = false;
			}
			break;
		case ENTERO:
			if (v1->dato.datoInt != 0) {
				valor = false;
			}
			break;
		case DOBLE:
			if (v1->dato.datoDouble != 0) {
				valor = false;
			}
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_not)" << endl;
			return false;
	}

	if (valor == true) {
		push_stack(1);
	} else {
		push_stack(0);
	}

	return true;
}

bool asm_and() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			compareResult = v1->dato.datoChar && v2->dato.datoChar;
			break;
		case ENTERO:
			compareResult = v1->dato.datoInt && v2->dato.datoInt;
			break;
		case DOBLE:
			compareResult = v1->dato.datoDouble && v2->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_and)" << endl;
			return false;
	}

	push_stack(compareResult);

	return true;
}

bool asm_or() {
	if (ajustar_stack() == false) {
		return false;
	}

	STACK* v2 = pop_stack();
	STACK* v1 = pop_stack();

	switch(v1->tipo) {
		case CARACTER:
			compareResult = v1->dato.datoChar || v2->dato.datoChar;
			break;
		case ENTERO:
			compareResult = v1->dato.datoInt || v2->dato.datoInt;
			break;
		case DOBLE:
			compareResult = v1->dato.datoDouble || v2->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_or)" << endl;
			return false;
	}
	
	push_stack(compareResult);

	return true;
}

bool asm_jmp() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\t_pc" << direccion;
	}

	programCounter = direccion;

	return true;
}

bool asm_jmpt() {
	unsigned short direccion = leer_direccion_codigo();

	STACK* ultimo = pop_stack();
	
	if (DEBUG == true) {
		cout << "\t_pc" << direccion;
	}

	compareResult = 0;
	switch(ultimo->tipo) {
		case CARACTER:
			compareResult = ultimo->dato.datoChar;
			break;
		case ENTERO:
			compareResult = ultimo->dato.datoInt;
			break;
		case DOBLE:
			compareResult = (int)ultimo->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_jmpt)" << endl;
			return false;
	}

	if (compareResult == false) {
		return true;
	}

	programCounter = direccion;

	return true;
}

bool asm_jmpf() {
	unsigned short direccion = leer_direccion_codigo();

	STACK* ultimo = pop_stack();

	if (DEBUG == true) {
		cout << "\t_pc" << direccion;
	}

	compareResult = 0;
	switch(ultimo->tipo) {
		case CARACTER:
			compareResult = ultimo->dato.datoChar;
			break;
		case ENTERO:
			compareResult = ultimo->dato.datoInt;
			break;
		case DOBLE:
			compareResult = (int)ultimo->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_jmpf)" << endl;
			return false;
	}

	if (compareResult == true) {
		return true;
	}

	programCounter = direccion;

	return true;
}

bool asm_readc() {
	unsigned short direccion = leer_direccion_codigo();

	char* direccion_datos = (char*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << endl;
	}

	do {
		cin.clear();
		cin >> direccion_datos[0];
	} while (cin.fail());

	return true;
}

bool asm_readi() {
	unsigned short direccion = leer_direccion_codigo();

	int* direccion_datos = (int*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << endl;
	}

	do {
		cin.clear();
		cin >> direccion_datos[0];
	} while (cin.fail());

	return true;
}

bool asm_readd() {
	unsigned short direccion = leer_direccion_codigo();

	double* direccion_datos = (double*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << endl;
	}

	do {
		cin.clear();
		cin >> direccion_datos[0];
	} while (cin.fail());

	return true;
}

bool asm_readvc() {
	unsigned short direccion = leer_direccion_codigo();

	char* direccion_datos = (char*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]" << endl;
	}

	do {
		cin.clear();
		cin >> direccion_datos[idx];
	} while (cin.fail());

	return true;
}

bool asm_readvi() {
	unsigned short direccion = leer_direccion_codigo();

	int* direccion_datos = (int*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]" << endl;
	}

	do {
		cin.clear();
		cin >> direccion_datos[idx];
	} while (cin.fail());

	return true;
}

bool asm_readvd() {
	unsigned short direccion = leer_direccion_codigo();

	double* direccion_datos = (double*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]" << endl;
	}

	do {
		cin.clear();
		cin >> direccion_datos[idx];
	} while (cin.fail());

	return true;
}

bool asm_reads() {
	unsigned short direccion = leer_direccion_codigo();

	char* direccion_datos = (char*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << endl;
	}

	string data;
	cin.clear();
	cin >> data;
	for (unsigned int i = 0; i < data.length(); i++) {
		direccion_datos[i] = data.c_str()[i];
	}
	direccion_datos[data.length()] = 0;

	return true;
}

bool asm_readvs() {
	unsigned short direccion = leer_direccion_codigo();

	char* direccion_datos = (char*)(data_segment + direccion + idx * size);

	if (DEBUG == true) {
		cout << "\tX" << direccion << endl;
	}

	string data;
	cin.clear();
	cin >> data;
	for (unsigned int i = 0; i < data.length(); i++) {
		direccion_datos[i] = data.c_str()[i];
	}
	direccion_datos[data.length()] = 0;

	return true;
}

bool asm_writec() {
	unsigned short direccion = leer_direccion_codigo();

	char* direccion_datos = (char*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << endl;
	}

	cout << direccion_datos[0];

	if (DEBUG == true) {
		cout << endl;
	}

	return true;
}

bool asm_writei() {
	unsigned short direccion = leer_direccion_codigo();

	int* direccion_datos = (int*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << endl;
	}

	cout << direccion_datos[0];

	if (DEBUG == true) {
		cout << endl;
	}

	return true;
}

bool asm_writed() {
	unsigned short direccion = leer_direccion_codigo();

	double* direccion_datos = (double*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << endl;
	}

	cout << direccion_datos[0];

	if (DEBUG == true) {
		cout << endl;
	}

	return true;
}

bool asm_writevc() {
	unsigned short direccion = leer_direccion_codigo();

	char* direccion_datos = (char*)(data_segment + direccion + idx);

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]" << endl;
	}

	cout << direccion_datos[0];

	if (DEBUG == true) {
		cout << endl;
	}

	return true;
}

bool asm_writevi() {
	unsigned short direccion = leer_direccion_codigo();

	int* direccion_datos = (int*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]" << endl;
	}

	cout << direccion_datos[idx];

	if (DEBUG == true) {
		cout << endl;
	}

	return true;
}

bool asm_writevd() {
	unsigned short direccion = leer_direccion_codigo();

	double* direccion_datos = (double*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]" << endl;
	}

	cout << direccion_datos[idx];

	if (DEBUG == true) {
		cout << endl;
	}

	return true;
}

bool asm_writecc() {
	if (DEBUG == true) {
		cout << endl;
	}
	
	cout << leer_caracter_codigo();

	if (DEBUG == true) {
		cout << endl;
	}

	return true;
}

bool asm_writeci() {
	if (DEBUG == true) {
		cout << endl;
	}

	cout << leer_entero_codigo();

	if (DEBUG == true) {
		cout << endl;
	}

	return true;
}

bool asm_writecd() {
	if (DEBUG == true) {
		cout << endl;
	}

	cout << leer_doble_codigo();

	if (DEBUG == true) {
		cout << endl;
	}

	return true;
}

bool asm_writes() {
	unsigned short direccion = leer_direccion_codigo();

	char* direccion_datos = (char*)(data_segment + direccion);

	if (DEBUG == true) {
		cout << "\tX" << direccion << endl;
	}

	cout << direccion_datos;

	if (DEBUG == true) {
		cout << endl;
	}

	return true;
}

bool asm_writem() {
	int tamanio = leer_caracter_codigo();
	char* datos = (char*)(code_segment + programCounter);

	programCounter += tamanio;

	char* c = new char[tamanio + 1];
	c[tamanio] = 0;

	for (int i = 0; i < tamanio; i++) {
		c[i] = datos[i];
	}

	if (DEBUG == true) {
		cout << "\t\"" << c << "\"" << endl;
	}

	cout << c;

	delete c;

	if (DEBUG == true) {
		cout << endl;
	}

	return true;
}

bool asm_writep() {
	STACK* ultimo = pop_stack();

	if (DEBUG == true) {
		cout << endl;
	}

	switch(ultimo->tipo) {
		case CARACTER:
			cout << ultimo->dato.datoChar;
			break;
		case ENTERO:
			cout << ultimo->dato.datoInt;
			break;
		case DOBLE:
			cout << ultimo->dato.datoDouble;
			break;
		case STRING:
			cout << *(ultimo->dato.datoString);
			break;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_writep)" << endl;
			return false;
	}

	return true;
}

bool asm_writecr() {
	cout << endl;

	return true;
}

bool asm_pushc() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion;
	}

	char* direccion_datos = (char*)(data_segment + direccion);

	push_stack(direccion_datos[0]);

	return true;
}

bool asm_pushi() {
	unsigned short direccion = leer_direccion_codigo();
	
	if (DEBUG == true) {
		cout << "\tX" << direccion;
	}

	int* direccion_datos = (int*)(data_segment + direccion);

	push_stack(direccion_datos[0]);

	return true;
}

bool asm_pushd() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion;
	}

	double* direccion_datos = (double*)(data_segment + direccion);

	push_stack(direccion_datos[0]);

	return true;
}

bool asm_pushvc() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]";
	}

	char* direccion_datos = (char*)(data_segment + direccion);

	push_stack(direccion_datos[idx]);

	return true;
}

bool asm_pushvi() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]";
	}

	int* direccion_datos = (int*)(data_segment + direccion);

	push_stack(direccion_datos[idx]);

	return true;
}

bool asm_pushvd() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]";
	}

	double* direccion_datos = (double*)(data_segment + direccion);

	push_stack(direccion_datos[idx]);

	return true;
}

bool asm_pushcc() {
	push_stack(leer_caracter_codigo());

	return true;
}

bool asm_pushci() {
	push_stack(leer_entero_codigo());

	return true;
}

bool asm_pushcd() {
	push_stack(leer_doble_codigo());

	return true;
}

bool asm_popc() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion;
	}

	char* direccion_datos = (char*)(data_segment + direccion);

	STACK* var = pop_stack();

	switch(var->tipo) {
		case CARACTER:
			direccion_datos[0] = var->dato.datoChar;
			break;
		case ENTERO:
			direccion_datos[0] = (char)var->dato.datoInt;
			break;
		case DOBLE:
			direccion_datos[0] = (char)var->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_popc)" << endl;
			return false;
	}

	return true;
}

bool asm_popi() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion;
	}

	int* direccion_datos = (int*)(data_segment + direccion);

	STACK* var = pop_stack();

	switch(var->tipo) {
		case CARACTER:
			direccion_datos[0] = (int)var->dato.datoChar;
			break;
		case ENTERO:
			direccion_datos[0] = var->dato.datoInt;
			break;
		case DOBLE:
			direccion_datos[0] = (int)var->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_popi)" << endl;
			return false;
	}

	return true;
}

bool asm_popd() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion;
	}

	double* direccion_datos = (double*)(data_segment + direccion);

	STACK* var = pop_stack();

	switch(var->tipo) {
		case CARACTER:
			direccion_datos[0] = (double)var->dato.datoChar;
			break;
		case ENTERO:
			direccion_datos[0] = (double)var->dato.datoInt;
			break;
		case DOBLE:
			direccion_datos[0] = var->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_popd)" << endl;
			return false;
	}

	return true;
}

bool asm_popvc() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]";
	}

	char* direccion_datos = (char*)(data_segment + direccion);

	STACK* var = pop_stack();

	switch(var->tipo) {
		case CARACTER:
			direccion_datos[idx] = var->dato.datoChar;
			break;
		case ENTERO:
			direccion_datos[idx] = (char)var->dato.datoInt;
			break;
		case DOBLE:
			direccion_datos[idx] = (char)var->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_popvc)" << endl;
			return false;
	}

	return true;
}

bool asm_popvi() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]";
	}

	int* direccion_datos = (int*)(data_segment + direccion);

	STACK* var = pop_stack();

	switch(var->tipo) {
		case CARACTER:
			direccion_datos[idx] = (int)var->dato.datoChar;
			break;
		case ENTERO:
			direccion_datos[idx] = var->dato.datoInt;
			break;
		case DOBLE:
			direccion_datos[idx] = (int)var->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_popvi)" << endl;
			return false;
	}

	return true;
}

bool asm_popvd() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion << "[" << idx << "]";
	}

	double* direccion_datos = (double*)(data_segment + direccion);

	STACK* var = pop_stack();

	switch(var->tipo) {
		case CARACTER:
			direccion_datos[idx] = (double)var->dato.datoChar;
			break;
		case ENTERO:
			direccion_datos[idx] = (double)var->dato.datoInt;
			break;
		case DOBLE:
			direccion_datos[idx] = var->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_popvd)" << endl;
			return false;
	}

	return true;
}

bool asm_popx() {
	STACK* var = pop_stack();

	switch(var->tipo) {
		case CARACTER:
			idx = (unsigned short)var->dato.datoChar;
			break;
		case ENTERO:
			idx = (unsigned short)var->dato.datoInt;
			break;
		case DOBLE:
			idx = (unsigned short)var->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_popx)" << endl;
			return false;
	}

	return true;
}

bool asm_popz() {
	STACK* var = pop_stack();

	switch(var->tipo) {
		case CARACTER:
			size = (unsigned short)var->dato.datoChar;
			break;
		case ENTERO:
			size = (unsigned short)var->dato.datoInt;
			break;
		case DOBLE:
			size = (unsigned short)var->dato.datoDouble;
			break;
		case STRING:
			cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (STRING)" << endl;
			return false;
		case CLEAR:
			cout << "\n[RUNTIME ERROR] valor nulo en el stack (asm_popz)" << endl;
			return false;
	}

	return true;
}

bool asm_pushm() {
	unsigned int direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion;
	}
	
	char* direccion_datos = (char*)(data_segment + direccion);

	string dato = "";
	unsigned int i = 0;

	while (true) {
		char c = direccion_datos[i++];
		if (c == 0) {
			break;
		}

		dato = dato + c; 
	}

	push_stack(dato);

	return true;
}

bool asm_pushvm() {
	unsigned int direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion;
	}
	
	char* direccion_datos = (char*)(data_segment + direccion + idx * size);

	string dato = "";
	unsigned int i = 0;

	while (true) {
		char c = direccion_datos[i++];
		if (c == 0) {
			break;
		}

		dato = dato + c; 
	}

	push_stack(dato);

	return true;
}

bool asm_popvm() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion;
	}

	char* direccion_datos = (char*)(data_segment + direccion + idx * size);

	STACK* var = pop_stack();

	if (var->tipo != STRING) {
		cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (!= STRING)" << endl;
		return false;
	}

	for (unsigned int i = 0; i < var->dato.datoString->length(); i++) {
		((char*)direccion_datos)[i] = (*var->dato.datoString)[i];
	}

	return true;
}

bool asm_popm() {
	unsigned short direccion = leer_direccion_codigo();

	if (DEBUG == true) {
		cout << "\tX" << direccion;
	}

	char* direccion_datos = (char*)(data_segment + direccion);

	STACK* var = pop_stack();

	if (var->tipo != STRING) {
		cout << "\n[RUNTIME ERROR] tipo de operacion invalida sobre el stack (!= STRING)" << endl;
		return false;
	}

	for (unsigned int i = 0; i < var->dato.datoString->length(); i++) {
		((char*)direccion_datos)[i] = (*var->dato.datoString)[i];
	}

	return true;
}

bool asm_pushcm() {
	int tamanio = leer_caracter_codigo();
	char* datos = (char*)(code_segment + programCounter);

	programCounter += tamanio;

	string dato = "";

	for (int i = 0; i < tamanio; i++) {
		dato = dato + datos[i];
	}

	push_stack(dato);

	return true;
}

INSTRUCCION* instrucciones[63] = {
		crear_instruccion("HLT", asm_hlt),
		crear_instruccion("ADD", asm_add),
		crear_instruccion("SUB", asm_sub),
		crear_instruccion("MUL", asm_mul),
		crear_instruccion("DIV", asm_div),
		crear_instruccion("MOD", asm_mod),
		crear_instruccion("INC", asm_inc),
		crear_instruccion("DEC", asm_dec),
		crear_instruccion("CMPEQ", asm_cmpeq),
		crear_instruccion("CMPNE", asm_cmpne),
		crear_instruccion("CMPLT", asm_cmplt),
		crear_instruccion("CMPLE", asm_cmple),
		crear_instruccion("CMPGT", asm_cmpgt),
		crear_instruccion("CMPGE", asm_cmpge),
		crear_instruccion("NOT", asm_not),
		crear_instruccion("AND", asm_and),
		crear_instruccion("OR", asm_or),
		crear_instruccion("JMP", asm_jmp),
		crear_instruccion("JMPT", asm_jmpt),
		crear_instruccion("JMPF", asm_jmpf),
		crear_instruccion("READC", asm_readc),
		crear_instruccion("READI", asm_readi),
		crear_instruccion("READD", asm_readd),
		crear_instruccion("READVC", asm_readvc),
		crear_instruccion("READVI", asm_readvi),
		crear_instruccion("READVD", asm_readvd),
		crear_instruccion("READS", asm_reads),
		crear_instruccion("READVS", asm_readvs),
		crear_instruccion("WRITEC", asm_writec),
		crear_instruccion("WRITEI", asm_writei),
		crear_instruccion("WRITED", asm_writed),
		crear_instruccion("WRITEVC", asm_writevc),
		crear_instruccion("WRITEVI", asm_writevi),
		crear_instruccion("WRITEVD", asm_writevd),
		crear_instruccion("WRITECC", asm_writecc),
		crear_instruccion("WRITECI", asm_writeci),
		crear_instruccion("WRITECD", asm_writecd),
		crear_instruccion("WRITES", asm_writes),
		crear_instruccion("WRITEM", asm_writem),
		crear_instruccion("WRITEP", asm_writep),
		crear_instruccion("WRITECR", asm_writecr),
		crear_instruccion("PUSHC", asm_pushc),
		crear_instruccion("PUSHI", asm_pushi),
		crear_instruccion("PUSHD", asm_pushd),
		crear_instruccion("PUSHVC", asm_pushvc),
		crear_instruccion("PUSHVI", asm_pushvi),
		crear_instruccion("PUSHVD", asm_pushvd),
		crear_instruccion("PUSHVM", asm_pushvm),
		crear_instruccion("PUSHM", asm_pushm),
		crear_instruccion("PUSHCC", asm_pushcc),
		crear_instruccion("PUSHCI", asm_pushci),
		crear_instruccion("PUSHCD", asm_pushcd),
		crear_instruccion("PUSHCM", asm_pushcm),
		crear_instruccion("POPC", asm_popc),
		crear_instruccion("POPI", asm_popi),
		crear_instruccion("POPD", asm_popd),
		crear_instruccion("POPVC", asm_popvc),
		crear_instruccion("POPVI", asm_popvi),
		crear_instruccion("POPVD", asm_popvd),
		crear_instruccion("POPVM", asm_popvm),
		crear_instruccion("POPM", asm_popm),
		crear_instruccion("POPX", asm_popx),
		crear_instruccion("POPZ", asm_popz)
	};

void memdmp() {
	FILE* dmp = fopen("mem.dmp", "w+b");
	fwrite(&data_segment, dataSegmentSize, 1, dmp);
	fclose(dmp);
}

bool ejecutar() {
	while (HALT == false) {
		if (DEBUG == true) {
			memdmp();
		}
		char codigo = code_segment[programCounter];
		if (codigo > 62 || codigo < 0) {
			cout << "[ERROR] codigo invalido " << (int)codigo << " en PC: " << programCounter << endl;
			return false;
		}

		INSTRUCCION* instruccion = instrucciones[(int)codigo];

		if (DEBUG == true) {
			cout << "\t\t\t[CHASM]\t_pc" << programCounter << ":\t" << instruccion->instruccion;
		}

		programCounter++;
		if (instruccion->function() == false) {
			cout << "[ERROR] error en el programa" << endl;
		}
		
		if (DEBUG == true) {
			cout << endl;
		}
	}
	
	return true;
}

void liberar_memoria() {
	// libera los String que hayan faltado :)
	for (stackPointer = 0; stackPointer < MAX_STACK; stackPointer++) {
		delete_string();
	}
	
	delete data_segment;
	delete code_segment;
	delete stack;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		cout << "Modo de uso:" << endl;
		cout << "    ch <archivo> [DEBUG]" << endl;
		return EXIT_FAILURE;
	}
	cout << "CHVM (c) 2007 Hilario Perez Corona" << endl;
	cout << "V58.1" << endl;
	cout << endl;

	if (argc > 2) {
		DEBUG = true;
	}

	entrada = fopen(argv[1], "rb");

	if (entrada == NULL) {
		cout << "error, el archivo no existe :(" << endl;
		return EXIT_FAILURE;
	}

	fseek(entrada, 12, SEEK_SET);

	
	fread(&dataSegmentSize, sizeof(short), 1, entrada);
	sysmsg("data segment size:");
	sysmsg(dataSegmentSize);
	
	fread(&codeSegmentSize, sizeof(short), 1, entrada);
	sysmsg("code segment size:");
	sysmsg(codeSegmentSize);

	sysmsg("solicitando memoria");
	stack = new STACK[MAX_STACK];
	data_segment = new char[dataSegmentSize];
	code_segment = new char[codeSegmentSize];

	sysmsg("preparando segmento de datos");
	for (unsigned int i = 0; i < dataSegmentSize; i++) {
		data_segment[i] = 0;
	}

	sysmsg("cargando segmento de codigo");
	fread(code_segment, sizeof(char), codeSegmentSize, entrada);

	fclose(entrada);

	sysmsg("inicializando registros");
	stackPointer = 0;
	programCounter = 0;
	compareResult = false;
	HALT = false;
	idx = 0;

	sysmsg("ejecutando programa");

	if (ejecutar() == false) {
		cout << endl;
		cout << "[ERROR] error en la ejecucion :(" << endl;
		liberar_memoria();

		return EXIT_FAILURE;
	}
	liberar_memoria();
	cout << endl;

	return EXIT_SUCCESS;
}
