#ifndef GC_LLVM_H_
#define GC_LLVM_H_

#include <iostream>
#include "tokens.h"
#include "parser.h"
#include "types.h"
#include "ch_asm.h"

using namespace std;

namespace llvm {
	class Function;
	class FunctionType;
	class CallInst;
	class Value;
	class Instruction;
	class Type;
	class StoreInst;
}

struct PROGRAMA_LLVM;
typedef struct PROGRAMA_LLVM PROGRAMA_LLVM;

void generar_codigo_llvm(PRODUCCION* ch, TABLA_SIMBOLOS* simbolos, string salida);

PROGRAMA_LLVM* llvm_crear(string nombre, TABLA_SIMBOLOS* simbolos);
void llvm_cerrar(PROGRAMA_LLVM* programa, string salida);
void llvm_inicializar(PROGRAMA_LLVM* programa);
void llvm_declarar_string_api(PROGRAMA_LLVM* programa);
llvm::Function* llvm_declarar_funcion(PROGRAMA_LLVM* programa, llvm::FunctionType* type, const char* name);
llvm::CallInst* llvm_call_0(PROGRAMA_LLVM* programa, llvm::Function* function);
llvm::CallInst* llvm_call_1(PROGRAMA_LLVM* programa, llvm::Function* function, llvm::Value* v1);
llvm::CallInst* llvm_call_2(PROGRAMA_LLVM* programa, llvm::Function* function, llvm::Value* v1, llvm::Value* v2);
llvm::CallInst* llvm_call_3(PROGRAMA_LLVM* programa, llvm::Function* function, llvm::Value* v1, llvm::Value* v2, llvm::Value* v3);

void llvm_generar_ch(PRODUCCION *ch, PROGRAMA_LLVM* programa);
void llvm_generar_hijos(PRODUCCION *produccion_padre, PROGRAMA_LLVM* programa);
void llvm_generar_produccion(PRODUCCION *produccion, PROGRAMA_LLVM* programa);

void llvm_generar_produccion_escritura(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
llvm::Value* llvm_generar_produccion_expresion_string(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
llvm::Value* llvm_generar_produccion_expresion_string_rr(PRODUCCION *produccion, PROGRAMA_LLVM* programa, llvm::Value* prev);
llvm::Value* llvm_generar_produccion_string(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
llvm::Value* llvm_generar_produccion_expresion_numero(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
llvm::Value* llvm_generar_produccion_expresion_numero_rr(PRODUCCION *produccion, PROGRAMA_LLVM* programa, llvm::Value* prev);
llvm::Value* llvm_generar_produccion_termino(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
llvm::Value* llvm_generar_produccion_termino_rr(PRODUCCION *produccion, PROGRAMA_LLVM* programa, llvm::Value* prev);
llvm::Value* llvm_generar_produccion_factor(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
llvm::Value* llvm_generar_produccion_etiqueta(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
llvm::Value* llvm_asegurar_tipo(llvm::Value*, llvm::Type*, PROGRAMA_LLVM* programa);
void llvm_generar_produccion_asignacion(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
void llvm_generar_produccion_lectura(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
llvm::Value* llvm_generar_produccion_condicion(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
llvm::Value* llvm_generar_produccion_condicion_rr(PRODUCCION *produccion, PROGRAMA_LLVM* programa, llvm::Value* prev);
llvm::Value* llvm_generar_produccion_factor_condicion(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
void llvm_generar_produccion_if(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
void llvm_generar_produccion_while(PRODUCCION *produccion, PROGRAMA_LLVM* programa);
void llvm_generar_produccion_for(PRODUCCION *produccion, PROGRAMA_LLVM* programa);

#endif /*GC_LLVM_H_*/
