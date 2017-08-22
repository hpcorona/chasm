// LLVM
#include "gc_llvm.h"
#include "tokens.h"
#include "types.h"
#include <llvm/Pass.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/IR/Type.h>
#include <llvm/PassRegistry.h>
#include <llvm/InitializePasses.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/PassInfo.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LegacyPassManager.h>

struct PROGRAMA_LLVM {
    llvm::LLVMContext *contexto;
    std::unique_ptr<llvm::Module> modulo;
    llvm::Function *main;
    llvm::IRBuilder<> *builder;

    TABLA_SIMBOLOS *simbolos;

    map<string, llvm::LoadInst *> constantes;

    llvm::Function *fn_read;
    llvm::Function *fn_readinteger;
    llvm::Function *fn_readdouble;
    llvm::Function *fn_print;
    llvm::Function *fn_println;
    llvm::Function *fn_strdupl;
    llvm::Function *fn_strjoin;
    llvm::Function *fn_strassign;
    llvm::Function *fn_strdouble;
    llvm::Function *fn_strinteger;
    llvm::Function *fn_strlt;
    llvm::Function *fn_strle;
    llvm::Function *fn_streq;
    llvm::Function *fn_strgt;
    llvm::Function *fn_strge;

    llvm::Function *fn_integerstr;
    llvm::Function *fn_doublestr;
};

// Generado al momento de Compilar
extern char *get_runtime_bc();

extern unsigned int get_runtime_bc_size();

llvm::StringRef runtime_bc_ref(get_runtime_bc(), (size_t) get_runtime_bc_size());
unique_ptr<llvm::MemoryBuffer> buff = llvm::MemoryBuffer::getMemBuffer(runtime_bc_ref, "runtime_bc", false);

void generar_codigo_llvm(PRODUCCION *ch, TABLA_SIMBOLOS *simbolos, string salida) {
    PROGRAMA_LLVM *programa = llvm_crear(salida, simbolos);
    llvm_generar_ch(ch, programa);
    llvm_cerrar(programa, salida);

    delete programa;
}

void llvm_cerrar(PROGRAMA_LLVM *programa, string salida) {
    assert(programa != nullptr);
    assert(programa->contexto != nullptr);
    assert(programa->main != nullptr);
    assert(programa->builder != nullptr);

    if (verifyModule(*programa->modulo) == false) {

        std::error_code errinfo;
        std::string salidabc = salida + ".bc";
        llvm::raw_fd_ostream fs(salidabc, errinfo, llvm::sys::fs::OpenFlags::F_None);
        llvm::WriteBitcodeToFile(programa->modulo.get(), fs);

        programa->modulo->print(llvm::outs(), nullptr);
    } else {
        // Si hay error
        cerr << "[LLVM] error en la creación del código binario llvm";
    }
    programa->modulo.release();
    delete programa->contexto;
}

PROGRAMA_LLVM *llvm_crear(string nombre, TABLA_SIMBOLOS *simbolos) {
    assert(simbolos != NULL);

    PROGRAMA_LLVM *programa = new PROGRAMA_LLVM();
    programa->modulo = nullptr;
    programa->main = nullptr;
    programa->builder = nullptr;
    programa->fn_read = nullptr;
    programa->fn_readinteger = nullptr;
    programa->fn_readdouble = nullptr;
    programa->fn_print = nullptr;
    programa->fn_println = nullptr;
    programa->fn_strdupl = nullptr;
    programa->fn_strjoin = nullptr;
    programa->fn_strassign = nullptr;
    programa->fn_strdouble = nullptr;
    programa->fn_strinteger = nullptr;
    programa->fn_doublestr = nullptr;
    programa->fn_integerstr = nullptr;
    programa->fn_strlt = nullptr;
    programa->fn_strle = nullptr;
    programa->fn_streq = nullptr;
    programa->fn_strgt = nullptr;
    programa->fn_strge = nullptr;
    programa->contexto = nullptr;
    programa->modulo = nullptr;

    programa->simbolos = simbolos;

    llvm_inicializar(programa);

    return programa;
}

void llvm_inicializar(PROGRAMA_LLVM *programa) {
    assert(programa != nullptr);
    assert(programa->contexto == nullptr);
    assert(programa->simbolos != nullptr);

    programa->contexto = new llvm::LLVMContext();
    llvm::SMDiagnostic err;
    programa->modulo = llvm::parseIR(buff.get()->getMemBufferRef(), err, *programa->contexto);

    llvm_declarar_string_api(programa);

    TABLA_SIMBOLOS::iterator iterador;

    for (iterador = programa->simbolos->begin(); iterador != programa->simbolos->end(); iterador++) {
        SIMBOLO *simbolo = iterador->second;

        llvm::Type *type = NULL;
        int alignment = 0;
        int realAlignment = 0;
        switch (simbolo->tipo) {
            case DATO_INTEGER:
                type = llvm::Type::getInt32Ty(*programa->contexto);
                alignment = 4;
                break;
            case DATO_CHAR:
                type = llvm::Type::getInt8Ty(*programa->contexto);
                alignment = 1;
                break;
            case DATO_DOUBLE:
                type = llvm::Type::getDoubleTy(*programa->contexto);
                alignment = 8;
                break;
        }

        realAlignment = alignment;

        if (simbolo->dim2 != 0 && simbolo->dim1 != 0) {
            type = llvm::ArrayType::get(type, simbolo->dim2);
            alignment = 16;
        }

        if (simbolo->dim1 != 0) {
            type = llvm::ArrayType::get(type, simbolo->dim1);
            alignment = 16;
        }

        llvm::GlobalVariable *variable = new llvm::GlobalVariable(*programa->modulo, type, false,
                                                                  llvm::GlobalValue::CommonLinkage, 0, iterador->first);
        variable->setAlignment(alignment);

        if (simbolo->dim1 != 0) {
            llvm::ConstantAggregateZero *initializer = llvm::ConstantAggregateZero::get(type);
            variable->setInitializer(initializer);
        } else {
            if (simbolo->tipo == DATO_DOUBLE) {
                llvm::ConstantFP *initializer = llvm::ConstantFP::get(*programa->contexto, llvm::APFloat(0.0));
                variable->setInitializer(initializer);
            } else {
                llvm::ConstantInt *initializer = llvm::ConstantInt::get(*programa->contexto,
                                                                        llvm::APInt(alignment * 8, llvm::StringRef("0"),
                                                                                    10));
                variable->setInitializer(initializer);
            }
        }
        simbolo->variable = variable;
        simbolo->alignment = realAlignment;
    }

    programa->main = programa->modulo.get()->getFunction("ch_program");
    if (programa->main == nullptr) {
        llvm::ArrayRef<llvm::Type *> paramTypes;
        llvm::FunctionType *funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(*programa->contexto), paramTypes,
                                                               false);
        programa->main = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "ch_program",
                                                programa->modulo.get());
        programa->main->setCallingConv(llvm::CallingConv::C);

        llvm::BasicBlock *entry = llvm::BasicBlock::Create(*programa->contexto, "entry", programa->main, 0);
        programa->builder = new llvm::IRBuilder<>(entry);
    } else {
        programa->main->getEntryBlock().getFirstNonPHI()->removeFromParent();
        programa->builder = new llvm::IRBuilder<>(&programa->main->getEntryBlock());
    }
}

void llvm_declarar_string_api(PROGRAMA_LLVM *programa) {
    assert(programa != NULL);
    assert(programa->contexto != NULL);
    assert(programa->modulo != NULL);

    llvm::PointerType *charPtrTy = llvm::IntegerType::getInt8PtrTy(*programa->contexto, 0);
    llvm::IntegerType *boolTy = llvm::IntegerType::getInt8Ty(*programa->contexto);
    llvm::IntegerType *int32Ty = llvm::IntegerType::getInt32Ty(*programa->contexto);
    llvm::Type *doubleTy = llvm::Type::getDoubleTy(*programa->contexto);

    std::vector<llvm::Type *> params0;

    std::vector<llvm::Type *> params1;
    params1.push_back(charPtrTy);

    std::vector<llvm::Type *> params2;
    params2.push_back(charPtrTy);
    params2.push_back(charPtrTy);

    std::vector<llvm::Type *> params3;
    params3.push_back(charPtrTy);
    params3.push_back(int32Ty);
    params3.push_back(charPtrTy);

    std::vector<llvm::Type *> paramInt32;
    paramInt32.push_back(int32Ty);

    std::vector<llvm::Type *> paramDouble;
    paramDouble.push_back(doubleTy);

    llvm::FunctionType *strduplFn = llvm::FunctionType::get(charPtrTy, params1, false);
    llvm::FunctionType *strjoinFn = llvm::FunctionType::get(charPtrTy, params2, false);
    llvm::FunctionType *strassignFn = llvm::FunctionType::get(llvm::Type::getVoidTy(*programa->contexto), params3,
                                                              false);
    llvm::FunctionType *strdoubleFn = llvm::FunctionType::get(charPtrTy, paramDouble, false);
    llvm::FunctionType *strintegerFn = llvm::FunctionType::get(charPtrTy, paramInt32, false);
    llvm::FunctionType *integerstrFn = llvm::FunctionType::get(int32Ty, params1, false);
    llvm::FunctionType *doublestrFn = llvm::FunctionType::get(doubleTy, params1, false);

    llvm::FunctionType *strcmpFn = llvm::FunctionType::get(boolTy, params2, false);

    llvm::FunctionType *readFn = llvm::FunctionType::get(charPtrTy, params0, false);
    llvm::FunctionType *readintegerFn = llvm::FunctionType::get(int32Ty, params0, false);
    llvm::FunctionType *readdoubleFn = llvm::FunctionType::get(doubleTy, params0, false);
    llvm::FunctionType *printFn = llvm::FunctionType::get(llvm::Type::getVoidTy(*programa->contexto), params1, false);

    programa->fn_read = llvm_declarar_funcion(programa, readFn, "read");
    programa->fn_readinteger = llvm_declarar_funcion(programa, readintegerFn, "readinteger");
    programa->fn_readdouble = llvm_declarar_funcion(programa, readdoubleFn, "readdouble");
    programa->fn_print = llvm_declarar_funcion(programa, printFn, "print");
    programa->fn_println = llvm_declarar_funcion(programa, printFn, "println");

    programa->fn_strdupl = llvm_declarar_funcion(programa, strduplFn, "strdupl");
    programa->fn_strjoin = llvm_declarar_funcion(programa, strjoinFn, "strjoin");
    programa->fn_strassign = llvm_declarar_funcion(programa, strassignFn, "strassign");
    programa->fn_strdouble = llvm_declarar_funcion(programa, strdoubleFn, "strdouble");
    programa->fn_strinteger = llvm_declarar_funcion(programa, strintegerFn, "strinteger");
    programa->fn_doublestr = llvm_declarar_funcion(programa, doublestrFn, "doublestr");
    programa->fn_integerstr = llvm_declarar_funcion(programa, integerstrFn, "integerstr");

    programa->fn_strlt = llvm_declarar_funcion(programa, strcmpFn, "strlt");
    programa->fn_strle = llvm_declarar_funcion(programa, strcmpFn, "strle");
    programa->fn_streq = llvm_declarar_funcion(programa, strcmpFn, "streq");
    programa->fn_strgt = llvm_declarar_funcion(programa, strcmpFn, "strgt");
    programa->fn_strge = llvm_declarar_funcion(programa, strcmpFn, "strge");
}

llvm::Function *llvm_declarar_funcion(PROGRAMA_LLVM *programa, llvm::FunctionType *type, const char *name) {
    llvm::Function *ex = programa->modulo.get()->getFunction(name);
    if (ex == nullptr) {
        llvm::Function *fn = llvm::Function::Create(type, llvm::GlobalValue::ExternalLinkage, name,
                                                    programa->modulo.get());
        fn->setCallingConv(llvm::CallingConv::C);
        return fn;
    } else {
        return ex;
    }
}

llvm::CallInst *llvm_call_0(PROGRAMA_LLVM *programa, llvm::Function *function) {
    std::vector<llvm::Value *> params;

    llvm::CallInst *call = programa->builder->CreateCall(function, params);
    call->setCallingConv(llvm::CallingConv::C);
    call->setTailCall(false);

    return call;
}

llvm::CallInst *llvm_call_1(PROGRAMA_LLVM *programa, llvm::Function *function, llvm::Value *v1) {
    std::vector<llvm::Value *> params;
    params.push_back(v1);

    llvm::CallInst *call = programa->builder->CreateCall(function, params);
    call->setCallingConv(llvm::CallingConv::C);
    call->setTailCall(false);

    return call;
}

llvm::CallInst *llvm_call_2(PROGRAMA_LLVM *programa, llvm::Function *function, llvm::Value *v1, llvm::Value *v2) {
    std::vector<llvm::Value *> params;
    params.push_back(v1);
    params.push_back(v2);

    llvm::CallInst *call = programa->builder->CreateCall(function, params);
    call->setCallingConv(llvm::CallingConv::C);
    call->setTailCall(false);

    return call;
}

llvm::CallInst *llvm_call_3(PROGRAMA_LLVM *programa, llvm::Function *function, llvm::Value *v1, llvm::Value *v2,
                            llvm::Value *v3) {
    std::vector<llvm::Value *> params;
    params.push_back(v1);
    params.push_back(v2);
    params.push_back(v3);

    llvm::CallInst *call = programa->builder->CreateCall(function, params);
    call->setCallingConv(llvm::CallingConv::C);
    call->setTailCall(false);

    return call;
}

void llvm_generar_ch(PRODUCCION *ch, PROGRAMA_LLVM *programa) {
    assert(ch != NULL);
    assert(ch->tipo == PRODUCCION_CH);

    llvm_generar_hijos(ch, programa);
}

void llvm_generar_hijos(PRODUCCION *produccion_padre, PROGRAMA_LLVM *programa) {
    assert(produccion_padre != NULL);

    PRODUCCION *hijo = produccion_padre->hijos;
    while (hijo != NULL) {
        llvm_generar_produccion(hijo, programa);

        hijo = hijo->siguiente;
    }
}

void llvm_generar_produccion(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    TOKEN *token = NULL;

    switch (produccion->tipo) {
        case PRODUCCION_DATOS:
        case PRODUCCION_DECLARACION_ARREGLO:
        case PRODUCCION_DECLARACION_VARIABLE:
        case PRODUCCION_TIPO_DATO:
            // Los datos no generan codigo
            return;
        case PRODUCCION_INSTRUCCIONES:
        case PRODUCCION_INSTRUCCION:
            llvm_generar_hijos(produccion, programa);
            break;
        case PRODUCCION_EXPRESION:
            assert(0);
            //llvm_generar_hijos(produccion, programa);
            break;
        case PRODUCCION_ASIGNACION:
            llvm_generar_produccion_asignacion(produccion, programa);
            break;
        case PRODUCCION_LECTURA:
            llvm_generar_produccion_lectura(produccion, programa);
            break;
        case PRODUCCION_ESCRITURA:
            llvm_generar_produccion_escritura(produccion, programa);
            break;
        case PRODUCCION_IF:
            llvm_generar_produccion_if(produccion, programa);
            break;
        case PRODUCCION_WHILE:
            llvm_generar_produccion_while(produccion, programa);
            break;
        case PRODUCCION_FOR:
            llvm_generar_produccion_for(produccion, programa);
            break;
        case PRODUCCION_ETIQUETA:
            throw "etiqueta";
            //llvm_generar_produccion_etiqueta(produccion, programa);
            break;
        case PRODUCCION_EXPRESION_STRING:
            llvm_generar_produccion_expresion_string(produccion, programa);
            break;
        case PRODUCCION_EXPRESION_STRING_RR:
            assert(0);
            ////llvm_generar_produccion_expresion_string_rr(produccion, programa);
            break;
        case PRODUCCION_EXPRESION_NUMERO:
            throw "expresion numero";
            //llvm_generar_produccion_expresion_numero(produccion, programa);
            break;
        case PRODUCCION_EXPRESION_NUMERO_RR:
            assert(0);
            ////llvm_generar_produccion_expresion_numero_rr(produccion, programa);
            break;
        case PRODUCCION_TERMINO:
            throw "termino";
            //llvm_generar_produccion_termino(produccion, programa);
            break;
        case PRODUCCION_TERMINO_RR:
            assert(0);
            ////llvm_generar_produccion_termino_rr(produccion, programa);
            break;
        case PRODUCCION_FACTOR:
            throw "factor";
            //llvm_generar_produccion_factor(produccion, programa);
            break;
        case PRODUCCION_CONDICION:
            throw "condicion";
            //llvm_generar_produccion_condicion(produccion, programa);
            break;
        case PRODUCCION_CONDICION_RR:
            assert(0);
            //llvm_generar_produccion_condicion_rr(produccion, programa);
            break;
        case PRODUCCION_FACTOR_CONDICION:
            assert(0);
            //llvm_generar_produccion_factor_condicion(produccion, programa);
            break;
        case PRODUCCION_STRING:
            throw "string";
            //llvm_generar_produccion_string(produccion, programa);
            break;
        case PRODUCCION_LEXEMA:
            token = produccion->token;
            switch (token->tipo) {
                case TOKEN_START:
                    break;
                case TOKEN_END:
                case TOKEN_STOP:
                    programa->builder->CreateRetVoid();
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

void llvm_generar_produccion_escritura(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_ESCRITURA);

    PRODUCCION *expresion = seleccionar_produccion(produccion, ".>>");
    assert(expresion != NULL);
    assert(expresion->tipo == PRODUCCION_EXPRESION);

    TOKEN *print = seleccionar_token(produccion, "..");
    assert(print != NULL);
    assert(print->tipo == TOKEN_PRINT || print->tipo == TOKEN_PRINTLN);

    llvm::Function *fun = programa->fn_print;
    if (print->tipo == TOKEN_PRINTLN) {
        fun = programa->fn_println;
    }

    llvm::Value *cadena = nullptr;
    PRODUCCION *expresion_real = seleccionar_produccion(expresion, ".");
    if (expresion_real->tipo == PRODUCCION_EXPRESION_STRING) {
        cadena = llvm_generar_produccion_expresion_string(expresion_real, programa);
    } else if (expresion_real->tipo == PRODUCCION_EXPRESION_NUMERO) {
        llvm::Value *num = llvm_generar_produccion_expresion_numero(expresion_real, programa);
        if (num->getType() == programa->builder->getInt32Ty()) {
            cadena = llvm_call_1(programa, programa->fn_strinteger, num);
        } else {
            cadena = llvm_call_1(programa, programa->fn_strdouble, num);
        }
    }
    llvm_call_1(programa, fun, cadena);
}

llvm::Value *llvm_generar_produccion_expresion_string(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_EXPRESION_STRING);

    PRODUCCION *strings = seleccionar_produccion(produccion, ".");
    assert(strings != NULL);
    assert(strings->tipo == PRODUCCION_STRING);

    llvm::Value *result = llvm_generar_produccion_string(strings, programa);

    PRODUCCION *rr = seleccionar_produccion(produccion, ".>");
    if (rr != NULL) {
        assert(rr->tipo == PRODUCCION_EXPRESION_STRING_RR);

        result = llvm_generar_produccion_expresion_string_rr(rr, programa, result);
    }

    return result;
}

llvm::Value *llvm_generar_produccion_expresion_string_rr(PRODUCCION *produccion, PROGRAMA_LLVM *programa,
                                                         llvm::Value *prev) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_EXPRESION_STRING_RR);

    PRODUCCION *concat = seleccionar_produccion(produccion, ".>");
    assert(concat != NULL);
    assert(concat->tipo == PRODUCCION_STRING || concat->tipo == PRODUCCION_EXPRESION_NUMERO);

    llvm::Value *right = NULL;
    if (concat->tipo == PRODUCCION_STRING) {
        right = llvm_generar_produccion_string(concat, programa);
    } else if (concat->tipo == PRODUCCION_EXPRESION_NUMERO) {
        llvm::Value *num = llvm_generar_produccion_expresion_numero(concat, programa);
        if (num->getType() == programa->builder->getInt32Ty()) {
            right = llvm_call_1(programa, programa->fn_strinteger, num);
        } else {
            right = llvm_call_1(programa, programa->fn_strdouble, num);
        }
    }

    llvm::Value *result = llvm_call_2(programa, programa->fn_strjoin, prev, right);

    PRODUCCION *rr = seleccionar_produccion(produccion, ".>>");
    if (rr != NULL) {
        assert(rr->tipo == PRODUCCION_EXPRESION_STRING_RR);

        result = llvm_generar_produccion_expresion_string_rr(rr, programa, result);
    }

    return result;
}

llvm::Value *llvm_generar_produccion_string(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_STRING);

    PRODUCCION *child = seleccionar_produccion(produccion, ".");
    assert(child != NULL);
    assert(child->tipo == PRODUCCION_LEXEMA || child->tipo == PRODUCCION_ETIQUETA);

    if (child->tipo == PRODUCCION_ETIQUETA) {
        TOKEN *token_etiqueta = seleccionar_token(produccion, "..");
        assert(token_etiqueta != NULL);

        SIMBOLO *variable = (*(programa->simbolos))[token_etiqueta->token];
        assert(variable != NULL);

        llvm::GlobalVariable *glovar = (llvm::GlobalVariable *) variable->variable;
        assert(glovar != NULL);

        llvm::Value *index = llvm_generar_produccion_etiqueta(child, programa);

        if (index != NULL) {
            llvm::Value *zero = programa->builder->getInt32(0);
            vector<llvm::Value *> args;
            args.push_back(zero);
            args.push_back(zero);
            llvm::Value *arrPtr = programa->builder->CreateInBoundsGEP(index, args);

            return llvm_call_1(programa, programa->fn_strdupl,
                               programa->builder->CreatePointerCast(arrPtr, programa->builder->getInt8PtrTy(0)));
        } else {
            llvm::Value *zero = programa->builder->getInt32(0);
            vector<llvm::Value *> args;
            args.push_back(zero);
            args.push_back(zero);
            return llvm_call_1(programa, programa->fn_strdupl,
                               programa->builder->CreateInBoundsGEP(glovar->getValueType(), glovar, args));
        }
    } else {
        TOKEN *token_string = seleccionar_token(produccion, ".");

        string token = token_string->token.substr(1, token_string->token.length() - 2);
        return llvm_call_1(programa, programa->fn_strdupl, programa->builder->CreateGlobalStringPtr(token));
    }
}

llvm::Value *llvm_generar_produccion_expresion_numero(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_EXPRESION_NUMERO);

    PRODUCCION *termino = seleccionar_produccion(produccion, ".");
    assert(termino != NULL);
    assert(termino->tipo == PRODUCCION_TERMINO);

    llvm::Value *result = llvm_generar_produccion_termino(termino, programa);

    PRODUCCION *rr = seleccionar_produccion(produccion, ".>");
    if (rr != NULL) {
        assert(rr->tipo == PRODUCCION_EXPRESION_NUMERO_RR);

        result = llvm_generar_produccion_expresion_numero_rr(rr, programa, result);
    }

    return result;
}

llvm::Value *llvm_asegurar_tipo(llvm::Value *valor, llvm::Type *tipo, PROGRAMA_LLVM *programa) {
    if (valor->getType() == tipo) {
        return valor;
    }

    if (tipo == programa->builder->getDoubleTy()) {
        return programa->builder->CreateSIToFP(valor, tipo);
    } else if (tipo == programa->builder->getInt32Ty() || tipo == programa->builder->getInt8Ty()) {
        if (valor->getType()->isDoubleTy()) {
            return programa->builder->CreateFPToSI(valor, tipo);
        }

        return programa->builder->CreateSExtOrTrunc(valor, tipo);
    }

    return valor;
}

llvm::Value *llvm_generar_produccion_expresion_numero_rr(PRODUCCION *produccion, PROGRAMA_LLVM *programa,
                                                         llvm::Value *prev) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_EXPRESION_NUMERO_RR);

    PRODUCCION *termino = seleccionar_produccion(produccion, ".>");
    assert(termino != NULL);
    assert(termino->tipo == PRODUCCION_TERMINO);

    llvm::Value *left = llvm_generar_produccion_termino(termino, programa);

    TOKEN *operacion = seleccionar_token(produccion, ".");
    assert(operacion != NULL);
    assert(operacion->tipo == TOKEN_ADD || operacion->tipo == TOKEN_REST);

    llvm::Value *result = NULL;
    if (prev->getType() == programa->builder->getDoubleTy() || left->getType() == programa->builder->getDoubleTy()) {
        prev = llvm_asegurar_tipo(prev, programa->builder->getDoubleTy(), programa);
        left = llvm_asegurar_tipo(left, programa->builder->getDoubleTy(), programa);

        if (operacion->tipo == TOKEN_ADD) {
            result = programa->builder->CreateFAdd(prev, left);
        } else {
            result = programa->builder->CreateFSub(prev, left);
        }
    } else {
        if (operacion->tipo == TOKEN_ADD) {
            result = programa->builder->CreateAdd(prev, left);
        } else {
            result = programa->builder->CreateSub(prev, left);
        }
    }

    PRODUCCION *rr = seleccionar_produccion(produccion, ".>>");
    if (rr != NULL) {
        assert(rr->tipo == PRODUCCION_EXPRESION_NUMERO_RR);

        result = llvm_generar_produccion_expresion_numero_rr(rr, programa, result);
    }

    return result;
}

llvm::Value *llvm_generar_produccion_termino(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_TERMINO);

    PRODUCCION *factor = seleccionar_produccion(produccion, ".");
    assert(factor != NULL);
    assert(factor->tipo == PRODUCCION_FACTOR);

    llvm::Value *result = llvm_generar_produccion_factor(factor, programa);

    PRODUCCION *rr = seleccionar_produccion(produccion, ".>");
    if (rr != NULL) {
        assert(rr->tipo == PRODUCCION_TERMINO_RR);

        result = llvm_generar_produccion_termino_rr(rr, programa, result);
    }

    return result;
}

llvm::Value *llvm_generar_produccion_termino_rr(PRODUCCION *produccion, PROGRAMA_LLVM *programa, llvm::Value *prev) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_TERMINO_RR);

    PRODUCCION *factor = seleccionar_produccion(produccion, ".>");
    assert(factor != NULL);
    assert(factor->tipo == PRODUCCION_FACTOR);

    llvm::Value *left = llvm_generar_produccion_factor(factor, programa);

    TOKEN *operacion = seleccionar_token(produccion, ".");
    assert(operacion != NULL);
    assert(operacion->tipo == TOKEN_MUL || operacion->tipo == TOKEN_DIV || operacion->tipo == TOKEN_MOD);

    llvm::Value *result = NULL;
    // TODO: Upgrade variables a Double de ser necesario
    if (operacion->tipo != TOKEN_MOD && (prev->getType()->isDoubleTy() || left->getType()->isDoubleTy())) {
        prev = llvm_asegurar_tipo(prev, programa->builder->getDoubleTy(), programa);
        left = llvm_asegurar_tipo(left, programa->builder->getDoubleTy(), programa);

        if (operacion->tipo == TOKEN_MUL) {
            result = programa->builder->CreateFMul(prev, left);
        } else if (operacion->tipo == TOKEN_DIV) {
            result = programa->builder->CreateFDiv(prev, left);
        }
    } else {
        prev = llvm_asegurar_tipo(prev, programa->builder->getInt32Ty(), programa);
        left = llvm_asegurar_tipo(left, programa->builder->getInt32Ty(), programa);

        if (operacion->tipo == TOKEN_MUL) {
            result = programa->builder->CreateMul(prev, left);
        } else if (operacion->tipo == TOKEN_DIV) {
            result = programa->builder->CreateSDiv(prev, left);
        } else {
            result = programa->builder->CreateSRem(prev, left);
        }
    }

    PRODUCCION *rr = seleccionar_produccion(produccion, ".>>");
    if (rr != NULL) {
        assert(rr->tipo == PRODUCCION_TERMINO_RR);

        result = llvm_generar_produccion_termino_rr(rr, programa, result);
    }

    return result;
}

llvm::Value *llvm_generar_produccion_factor(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_FACTOR);

    bool resta = false;
    PRODUCCION *var = NULL;

    var = seleccionar_produccion(produccion, ".");
    assert(var != NULL);

    if (var->tipo == PRODUCCION_LEXEMA) {
        TOKEN *token = var->token;
        if (token->tipo == TOKEN_LPAR) {
            var = seleccionar_produccion(produccion, ".>");
            assert(var != NULL);
            assert(var->tipo == PRODUCCION_EXPRESION_NUMERO);

            return llvm_generar_produccion_expresion_numero(var, programa);
        } else if (token->tipo == TOKEN_REST) {
            resta = true;

            var = seleccionar_produccion(produccion, ".>");
        }
    }

    assert(var != NULL);
    assert(var->tipo == PRODUCCION_LEXEMA || var->tipo == PRODUCCION_ETIQUETA);

    if (var->tipo == PRODUCCION_LEXEMA) {
        TOKEN *token = var->token;
        int x;
        double y;

        switch (token->tipo) {
            case TOKEN_ENTERO:
                x = atoi(token->token.c_str());
                if (resta == true) {
                    x = -x;
                }

                return programa->builder->getInt32(x);
                break;
            case TOKEN_NUMERO:
                y = atof(token->token.c_str());
                if (resta == true) {
                    y = -y;
                }

                return llvm::ConstantFP::get(programa->builder->getDoubleTy(), y);
                break;
            default:
                assert(0);
                break;
        }
    } else {
        TOKEN *etiqueta = seleccionar_token(var, ".");
        assert(etiqueta != NULL);

        SIMBOLO *variable = (*(programa->simbolos))[etiqueta->token];
        assert(variable != NULL);

        llvm::GlobalVariable *glovar = (llvm::GlobalVariable *) variable->variable;

        llvm::Value *result = NULL;

        llvm::Value *index = llvm_generar_produccion_etiqueta(var, programa);
        if (index != NULL) {
            llvm::Type *type = index->getType()->getContainedType(index->getType()->getNumContainedTypes() - 1);
            llvm::Value *number = programa->builder->CreateLoad(type, index);

            if (type->isIntegerTy()) {
                result = llvm_asegurar_tipo(number, programa->builder->getInt32Ty(), programa);
            } else {
                result = llvm_asegurar_tipo(number, programa->builder->getDoubleTy(), programa);
            }
        } else {
            result = programa->builder->CreateLoad(glovar->getValueType(), glovar);
        }

        return result;
    }
}

llvm::Value *llvm_generar_produccion_etiqueta(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_ETIQUETA);

    TOKEN *token_etiqueta = seleccionar_token(produccion, ".");
    assert(token_etiqueta != NULL);

    SIMBOLO *variable = (*(programa->simbolos))[token_etiqueta->token];
    assert(variable != NULL);
    assert(variable->variable != NULL);

    llvm::GlobalVariable *glovar = (llvm::GlobalVariable *) variable->variable;
    assert(glovar != NULL);

    PRODUCCION *exprDim1 = seleccionar_produccion(produccion, ".>>");
    PRODUCCION *exprDim2 = seleccionar_produccion(produccion, ".>>>>");

    if (exprDim1 == NULL) {
        assert(exprDim2 == NULL);
        // no hacer nada :)

        return NULL;
    } else {
        if (exprDim2 == NULL) {
            // es una variable asi: var[n]
            assert(exprDim1->tipo == PRODUCCION_EXPRESION_NUMERO);

            llvm::Value *idx = llvm_generar_produccion_expresion_numero(exprDim1, programa);
            llvm::Value *idx_pos = programa->builder->CreateSExt(idx, programa->builder->getInt64Ty());

            std::vector<llvm::Value *> ptr_arr;
            ptr_arr.push_back(programa->builder->getInt64(0));
            ptr_arr.push_back(idx_pos);
            return programa->builder->CreateInBoundsGEP(glovar, ptr_arr);
        } else {
            // es una variable asi: var[n,m]
            assert(exprDim1->tipo == PRODUCCION_EXPRESION_NUMERO);
            assert(exprDim2->tipo == PRODUCCION_EXPRESION_NUMERO);

            llvm::Value *idx1 = llvm_asegurar_tipo(llvm_generar_produccion_expresion_numero(exprDim1, programa),
                                                   programa->builder->getInt32Ty(), programa);

            llvm::Value *idx1_pos = programa->builder->CreateSExt(idx1, programa->builder->getInt64Ty());

            std::vector<llvm::Value *> ptr1_arr;
            ptr1_arr.push_back(programa->builder->getInt64(0));
            ptr1_arr.push_back(idx1_pos);
            llvm::Value *dim1 = programa->builder->CreateInBoundsGEP(glovar, ptr1_arr);

            llvm::Value *idx2 = llvm_asegurar_tipo(llvm_generar_produccion_expresion_numero(exprDim2, programa),
                                                   programa->builder->getInt32Ty(), programa);

            llvm::Value *idx2_pos = programa->builder->CreateSExt(idx2, programa->builder->getInt64Ty());

            std::vector<llvm::Value *> ptr2_arr;
            ptr2_arr.push_back(programa->builder->getInt64(0));
            ptr2_arr.push_back(idx2_pos);
            return programa->builder->CreateInBoundsGEP(dim1, ptr2_arr);
        }
    }
}

void llvm_generar_produccion_asignacion(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_ASIGNACION);
    assert(programa != NULL);
    assert(programa->simbolos != NULL);

    PRODUCCION *expresion = seleccionar_produccion(produccion, ".>>");
    assert(expresion != NULL);
    assert(expresion->tipo == PRODUCCION_EXPRESION);

    llvm::Value *right = NULL;
    PRODUCCION *expresionReal = seleccionar_produccion(expresion, ".");
    if (expresionReal->tipo == PRODUCCION_EXPRESION_NUMERO) {
        right = llvm_generar_produccion_expresion_numero(expresionReal, programa);
    } else {
        right = llvm_generar_produccion_expresion_string(expresionReal, programa);
    }

    PRODUCCION *etiqueta = seleccionar_produccion(produccion, ".");
    assert(etiqueta != NULL);
    assert(etiqueta->tipo == PRODUCCION_ETIQUETA);

    TOKEN *token_etiqueta = seleccionar_token(etiqueta, ".");
    assert(token_etiqueta != NULL);

    SIMBOLO *dato = (*(programa->simbolos))[token_etiqueta->token];
    assert(dato != NULL);

    TIPO_RUNTIME tipo = tipo_etiqueta(programa->simbolos, etiqueta);

    llvm::GlobalVariable *glovar = (llvm::GlobalVariable *) dato->variable;

    llvm::Value *variable = llvm_generar_produccion_etiqueta(etiqueta, programa);
    if (variable != NULL) {
        if (tipo == RUNTIME_STRING) {
            std::vector<llvm::Value *> ptr_arr;
            ptr_arr.push_back(programa->builder->getInt32(0));
            ptr_arr.push_back(programa->builder->getInt32(0));

            llvm::Value *raw = programa->builder->CreateInBoundsGEP(variable, ptr_arr);
            llvm::Value *val = programa->builder->CreatePointerCast(raw, programa->builder->getInt8PtrTy(0));
            llvm_call_3(programa, programa->fn_strassign, val, programa->builder->getInt32(dato->dim2), right);
        } else {
            llvm::Type *type = variable->getType();
            if (!right->getType()->isPointerTy()) {
                right = llvm_asegurar_tipo(right, type->getContainedType(type->getNumContainedTypes() - 1), programa);
            }
            programa->builder->CreateStore(right, variable, false);
        }
    } else {
        if (tipo == RUNTIME_STRING) {
            std::vector<llvm::Value *> ptr_arr;
            ptr_arr.push_back(programa->builder->getInt32(0));
            ptr_arr.push_back(programa->builder->getInt32(0));

            llvm::Value *val = programa->builder->CreateInBoundsGEP(glovar, ptr_arr);
            llvm_call_3(programa, programa->fn_strassign, val, programa->builder->getInt32(dato->dim1), right);
        } else {
            llvm::Type *type = glovar->getValueType();
            if (!right->getType()->isPointerTy()) {
                right = llvm_asegurar_tipo(right, type, programa);
            }
            programa->builder->CreateStore(right, glovar, false);
        }
    }
}

void llvm_generar_produccion_lectura(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_LECTURA);
    assert(programa != NULL);
    assert(programa->simbolos != NULL);

    PRODUCCION *etiqueta = seleccionar_produccion(produccion, ".>>");
    assert(etiqueta != NULL);
    assert(etiqueta->tipo == PRODUCCION_ETIQUETA);

    TOKEN *token_etiqueta = seleccionar_token(etiqueta, ".");
    assert(token_etiqueta != NULL);

    SIMBOLO *dato = (*(programa->simbolos))[token_etiqueta->token];
    assert(dato != NULL);

    TIPO_RUNTIME tipo = tipo_etiqueta(programa->simbolos, etiqueta);
    llvm::GlobalVariable *glovar = (llvm::GlobalVariable *) dato->variable;

    llvm::Value *right = nullptr;

    llvm::Value *variable = llvm_generar_produccion_etiqueta(etiqueta, programa);
    if (variable != NULL) {
        if (tipo == RUNTIME_STRING) {
            right = llvm_call_0(programa, programa->fn_read);

            std::vector<llvm::Value *> ptr_arr;
            ptr_arr.push_back(programa->builder->getInt32(0));
            ptr_arr.push_back(programa->builder->getInt32(0));

            llvm::Value *raw = programa->builder->CreateInBoundsGEP(variable, ptr_arr);
            llvm::Value *val = programa->builder->CreatePointerCast(raw, programa->builder->getInt8PtrTy(0));
            llvm_call_3(programa, programa->fn_strassign, val, programa->builder->getInt32(dato->dim2), right);
        } else {
            llvm::Type *varType = variable->getType();
            llvm::Type *type = varType->getContainedType(varType->getNumContainedTypes() - 1);
            if (type == programa->builder->getInt32Ty()) {
                right = llvm_call_0(programa, programa->fn_readinteger);
            } else if (type == programa->builder->getInt8Ty()) {
                right = llvm_call_0(programa, programa->fn_readinteger);

                right = programa->builder->CreateSExtOrTrunc(right, type);
            } else if (type == programa->builder->getDoubleTy()) {
                right = llvm_call_0(programa, programa->fn_readdouble);
            }
            programa->builder->CreateStore(right, variable, false);
        }
    } else {
        if (tipo == RUNTIME_STRING) {
            right = llvm_call_0(programa, programa->fn_read);

            std::vector<llvm::Value *> ptr_arr;
            ptr_arr.push_back(programa->builder->getInt32(0));
            ptr_arr.push_back(programa->builder->getInt32(0));

            llvm::Value *val = programa->builder->CreateInBoundsGEP(glovar, ptr_arr);
            llvm_call_3(programa, programa->fn_strassign, val, programa->builder->getInt32(dato->dim1), right);
        } else {
            llvm::Type *type = glovar->getValueType();
            if (type == programa->builder->getInt32Ty()) {
                right = llvm_call_0(programa, programa->fn_readinteger);
            } else if (type == programa->builder->getInt8Ty()) {
                right = llvm_call_0(programa, programa->fn_readinteger);

                right = programa->builder->CreateSExtOrTrunc(right, type);
            } else if (type == programa->builder->getDoubleTy()) {
                right = llvm_call_0(programa, programa->fn_readdouble);
            }
            programa->builder->CreateStore(right, glovar, false);
        }
    }
}

llvm::Value *llvm_generar_produccion_condicion(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_CONDICION);

    PRODUCCION *factor_condicion = seleccionar_produccion(produccion, ".");
    assert(factor_condicion != NULL);
    assert(factor_condicion->tipo == PRODUCCION_FACTOR_CONDICION);

    llvm::Value *result = llvm_generar_produccion_factor_condicion(factor_condicion, programa);

    PRODUCCION *rr = seleccionar_produccion(produccion, ".>");
    if (rr != NULL) {
        assert(rr->tipo == PRODUCCION_CONDICION_RR);

        result = llvm_generar_produccion_condicion_rr(rr, programa, result);
    }

    return result;
}

llvm::Value *llvm_generar_produccion_condicion_rr(PRODUCCION *produccion, PROGRAMA_LLVM *programa, llvm::Value *prev) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_CONDICION_RR);

    PRODUCCION *factor_condicion = seleccionar_produccion(produccion, ".>");
    assert(factor_condicion != NULL);
    assert(factor_condicion->tipo == PRODUCCION_FACTOR_CONDICION);

    llvm::Value *result = llvm_generar_produccion_factor_condicion(factor_condicion, programa);

    PRODUCCION *unions = seleccionar_produccion(produccion, ".");
    assert(unions != NULL);
    assert(unions->tipo == PRODUCCION_UNION);

    TOKEN *lexema = seleccionar_token(unions, ".");
    assert(lexema != NULL);
    assert(lexema->tipo == TOKEN_AND || lexema->tipo == TOKEN_OR);

    if (lexema->tipo == TOKEN_AND) {
        result = programa->builder->CreateAnd(prev, result);
    } else {
        result = programa->builder->CreateOr(prev, result);
    }

    PRODUCCION *rr = seleccionar_produccion(produccion, ".>>");
    if (rr != NULL) {
        assert(rr->tipo == PRODUCCION_CONDICION_RR);

        result = llvm_generar_produccion_condicion_rr(rr, programa, result);
    }

    return result;
}

llvm::Value *llvm_generar_produccion_factor_condicion(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_FACTOR_CONDICION);

    PRODUCCION *operador = seleccionar_produccion(produccion, ".>");
    assert(operador != NULL);
    assert(operador->tipo == PRODUCCION_CONDICION || operador->tipo == PRODUCCION_OPERADOR);

    if (operador->tipo == PRODUCCION_CONDICION) {
        llvm_generar_produccion(operador, programa);
    } else {
        PRODUCCION *expr1 = seleccionar_produccion(produccion, ".");
        PRODUCCION *expr2 = seleccionar_produccion(produccion, ".>>");
        assert(expr1 != NULL);
        assert(expr1->tipo == PRODUCCION_EXPRESION_NUMERO || expr1->tipo == PRODUCCION_EXPRESION_STRING);
        assert(expr2 != NULL);
        assert(expr2->tipo == PRODUCCION_EXPRESION_STRING || expr2->tipo == PRODUCCION_EXPRESION_NUMERO);

        bool manejoString = false;
        if (expr1->tipo == PRODUCCION_EXPRESION_STRING || expr2->tipo == PRODUCCION_EXPRESION_STRING) {
            manejoString = true;
        }

        llvm::Value *v1 = nullptr;
        llvm::Value *v2 = nullptr;

        if (expr1->tipo == PRODUCCION_EXPRESION_STRING) {
            v1 = llvm_generar_produccion_expresion_string(expr1, programa);
        } else {
            v1 = llvm_generar_produccion_expresion_numero(expr1, programa);

            if (manejoString) {
                if (v1->getType()->isDoubleTy()) {
                    v1 = llvm_call_1(programa, programa->fn_strdouble, v1);
                } else {
                    v1 = llvm_call_1(programa, programa->fn_strinteger, v1);
                }
            }
        }

        if (expr2->tipo == PRODUCCION_EXPRESION_STRING) {
            v2 = llvm_generar_produccion_expresion_string(expr2, programa);
        } else {
            v2 = llvm_generar_produccion_expresion_numero(expr2, programa);

            if (manejoString) {
                if (v2->getType()->isDoubleTy()) {
                    v2 = llvm_call_1(programa, programa->fn_strdouble, v2);
                } else {
                    v2 = llvm_call_1(programa, programa->fn_strinteger, v2);
                }
            }
        }


        TOKEN *token_operador = seleccionar_token(operador, ".");
        assert(token_operador != NULL);

        if (manejoString) {
            switch (token_operador->tipo) {
                case TOKEN_EQ:
                    return programa->builder->CreateICmpEQ(llvm_call_2(programa, programa->fn_streq, v1, v2),
                                                           programa->builder->getInt8(1));
                case TOKEN_LT:
                    return programa->builder->CreateICmpEQ(llvm_call_2(programa, programa->fn_strlt, v1, v2),
                                                           programa->builder->getInt8(1));
                case TOKEN_LE:
                    return programa->builder->CreateICmpEQ(llvm_call_2(programa, programa->fn_strle, v1, v2),
                                                           programa->builder->getInt8(1));
                case TOKEN_GT:
                    return programa->builder->CreateICmpEQ(llvm_call_2(programa, programa->fn_strgt, v1, v2),
                                                           programa->builder->getInt8(1));
                case TOKEN_GE:
                    return programa->builder->CreateICmpEQ(llvm_call_2(programa, programa->fn_strge, v1, v2),
                                                           programa->builder->getInt8(1));
                case TOKEN_NE:
                    return programa->builder->CreateICmpNE(llvm_call_2(programa, programa->fn_streq, v1, v2),
                                                           programa->builder->getInt8(1));
                default:
                    assert(0);
                    break;
            }
        } else {
            if (v1->getType()->isDoubleTy() || v2->getType()->isDoubleTy()) {
                v1 = llvm_asegurar_tipo(v1, programa->builder->getDoubleTy(), programa);
                v2 = llvm_asegurar_tipo(v2, programa->builder->getDoubleTy(), programa);

                switch (token_operador->tipo) {
                    case TOKEN_EQ:
                        return programa->builder->CreateFCmpUEQ(v1, v2);
                    case TOKEN_LT:
                        return programa->builder->CreateFCmpULT(v1, v2);
                    case TOKEN_LE:
                        return programa->builder->CreateFCmpULE(v1, v2);
                    case TOKEN_GT:
                        return programa->builder->CreateFCmpUGT(v1, v2);
                    case TOKEN_GE:
                        return programa->builder->CreateFCmpUGE(v1, v2);
                    case TOKEN_NE:
                        return programa->builder->CreateFCmpUNE(v1, v2);
                    default:
                        assert(0);
                        break;
                }
            } else {
                v1 = llvm_asegurar_tipo(v1, programa->builder->getInt32Ty(), programa);
                v2 = llvm_asegurar_tipo(v2, programa->builder->getInt32Ty(), programa);

                switch (token_operador->tipo) {
                    case TOKEN_EQ:
                        return programa->builder->CreateICmpEQ(v1, v2);
                    case TOKEN_LT:
                        return programa->builder->CreateICmpSLT(v1, v2);
                    case TOKEN_LE:
                        return programa->builder->CreateICmpSLE(v1, v2);
                    case TOKEN_GT:
                        return programa->builder->CreateICmpSGT(v1, v2);
                    case TOKEN_GE:
                        return programa->builder->CreateICmpSGE(v1, v2);
                    case TOKEN_NE:
                        return programa->builder->CreateICmpNE(v1, v2);
                    default:
                        assert(0);
                        break;
                }
            }
        }
    }

    return nullptr;
}

void llvm_generar_produccion_if(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_IF);

    PRODUCCION *condicion = seleccionar_produccion(produccion, ".>>");
    assert(condicion != NULL);
    assert(condicion->tipo == PRODUCCION_CONDICION);

    llvm::Value *condition = llvm_generar_produccion_condicion(condicion, programa);

    PRODUCCION *instrucciones = seleccionar_produccion(produccion, ".>>>>");
    assert(instrucciones != NULL);
    assert(instrucciones->tipo == PRODUCCION_INSTRUCCIONES);

    TOKEN *elses = seleccionar_token(produccion, ".>>>>>");
    assert(elses != NULL);
    assert(elses->tipo == TOKEN_ELSE || elses->tipo == TOKEN_ENDIF);

    // Para poder volver
    llvm::BasicBlock *restoreBlock = programa->builder->GetInsertBlock();
    llvm::BasicBlock::iterator restorePoint = programa->builder->GetInsertPoint();

    llvm::BasicBlock *trueBlock = llvm::BasicBlock::Create(*programa->contexto, "if_true");
    llvm::BasicBlock *elseBlock = nullptr;
    llvm::BasicBlock *continuation = nullptr;
    if (elses->tipo == TOKEN_ELSE) {
        elseBlock = llvm::BasicBlock::Create(*programa->contexto, "else");
        continuation = llvm::BasicBlock::Create(*programa->contexto, "if_merge");
    } else {
        continuation = llvm::BasicBlock::Create(*programa->contexto, "if_merge");
        elseBlock = continuation;
    }

    programa->builder->CreateCondBr(condition, trueBlock, elseBlock);

    programa->main->getBasicBlockList().push_back(trueBlock);
    programa->builder->SetInsertPoint(trueBlock);
    llvm_generar_produccion(instrucciones, programa);
    programa->builder->CreateBr(continuation);
    trueBlock = programa->builder->GetInsertBlock();

    if (elses->tipo == TOKEN_ELSE) {
        PRODUCCION *instruccionesElse = seleccionar_produccion(produccion, ".>>>>>>");
        assert(instruccionesElse != NULL);
        assert(instruccionesElse->tipo == PRODUCCION_INSTRUCCIONES);

        programa->main->getBasicBlockList().push_back(elseBlock);
        programa->builder->SetInsertPoint(elseBlock);
        llvm_generar_produccion(instruccionesElse, programa);
        programa->builder->CreateBr(continuation);
        elseBlock = programa->builder->GetInsertBlock();
    }

    programa->main->getBasicBlockList().push_back(continuation);
    programa->builder->SetInsertPoint(continuation);
}

void llvm_generar_produccion_while(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_WHILE);

    PRODUCCION *condicion = seleccionar_produccion(produccion, ".>>");
    assert(condicion != NULL);
    assert(condicion->tipo == PRODUCCION_CONDICION);

    PRODUCCION *instrucciones = seleccionar_produccion(produccion, ".>>>>");
    assert(instrucciones != NULL);
    assert(instrucciones->tipo == PRODUCCION_INSTRUCCIONES);

    llvm::BasicBlock *whileTest = llvm::BasicBlock::Create(*programa->contexto, "while_test");
    llvm::BasicBlock *whileBlock = llvm::BasicBlock::Create(*programa->contexto, "while_block");
    llvm::BasicBlock *continuation = llvm::BasicBlock::Create(*programa->contexto, "while_merge");

    programa->builder->CreateBr(whileTest);

    programa->main->getBasicBlockList().push_back(whileTest);
    programa->builder->SetInsertPoint(whileTest);
    llvm::Value *condition = llvm_generar_produccion_condicion(condicion, programa);
    programa->builder->CreateCondBr(condition, whileBlock, continuation);

    programa->main->getBasicBlockList().push_back(whileBlock);
    programa->builder->SetInsertPoint(whileBlock);
    llvm_generar_produccion(instrucciones, programa);
    programa->builder->CreateBr(whileTest);

    programa->main->getBasicBlockList().push_back(continuation);
    programa->builder->SetInsertPoint(continuation);
}

void llvm_generar_produccion_for(PRODUCCION *produccion, PROGRAMA_LLVM *programa) {
    assert(produccion != NULL);
    assert(produccion->tipo == PRODUCCION_FOR);

    PRODUCCION *asignacion = seleccionar_produccion(produccion, ".>>");
    assert(asignacion != NULL);
    assert(asignacion->tipo == PRODUCCION_ASIGNACION);

    PRODUCCION *condicion = seleccionar_produccion(produccion, ".>>>>");
    assert(condicion != NULL);
    assert(condicion->tipo == PRODUCCION_CONDICION);

    PRODUCCION *instrucciones = seleccionar_produccion(produccion, ".>>>>>>>>");
    assert(instrucciones != NULL);
    assert(instrucciones->tipo == PRODUCCION_INSTRUCCIONES);

    PRODUCCION *incremento = seleccionar_produccion(produccion, ".>>>>>>");
    assert(incremento != NULL);
    assert(incremento->tipo == PRODUCCION_ASIGNACION);

    llvm::BasicBlock *forTest = llvm::BasicBlock::Create(*programa->contexto, "for_test");
    llvm::BasicBlock *forBlock = llvm::BasicBlock::Create(*programa->contexto, "for_block");
    llvm::BasicBlock *forIncrement = llvm::BasicBlock::Create(*programa->contexto, "for_increment");
    llvm::BasicBlock *continuation = llvm::BasicBlock::Create(*programa->contexto, "for_merge");

    llvm_generar_produccion_asignacion(asignacion, programa);
    programa->builder->CreateBr(forTest);

    programa->main->getBasicBlockList().push_back(forTest);
    programa->builder->SetInsertPoint(forTest);
    llvm::Value *condition = llvm_generar_produccion_condicion(condicion, programa);
    programa->builder->CreateCondBr(condition, forBlock, continuation);

    programa->main->getBasicBlockList().push_back(forBlock);
    programa->builder->SetInsertPoint(forBlock);
    llvm_generar_produccion(instrucciones, programa);
    programa->builder->CreateBr(forIncrement);
    forBlock = programa->builder->GetInsertBlock();

    programa->main->getBasicBlockList().push_back(forIncrement);
    programa->builder->SetInsertPoint(forIncrement);
    llvm_generar_produccion_asignacion(incremento, programa);
    programa->builder->CreateBr(forTest);

    programa->main->getBasicBlockList().push_back(continuation);
    programa->builder->SetInsertPoint(continuation);
}
