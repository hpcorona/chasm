#ifndef CH_ASM_H_
#define CH_ASM_H_

#define		CH_HLT		0
#define		CH_ADD		1
#define		CH_SUB		2
#define		CH_MUL		3
#define		CH_DIV		4
#define		CH_MOD		5
#define		CH_INC		6
#define		CH_DEC		7

#define		CH_CMPEQ	8
#define		CH_CMPNE	9
#define		CH_CMPLT	10
#define		CH_CMPLE	11
#define		CH_CMPGT	12
#define		CH_CMPGE	13

#define		CH_NOT		14

#define		CH_AND		15
#define		CH_OR		16

#define		CH_JMP		17
#define		CH_JMPT		18
#define		CH_JMPF		19

#define		CH_READC	20
#define		CH_READI	21
#define		CH_READD	22
#define		CH_READVC	23
#define		CH_READVI	24
#define		CH_READVD	25
#define		CH_READS	26
#define		CH_READVS	27

#define		CH_WRITEC	28
#define		CH_WRITEI	29
#define		CH_WRITED	30
#define		CH_WRITEVC	31
#define		CH_WRITEVI	32
#define		CH_WRITEVD	33
#define		CH_WRITECC	34
#define		CH_WRITECI	35
#define		CH_WRITECD	36
#define		CH_WRITES	37
#define		CH_WRITEM	38
#define		CH_WRITEP	39
#define		CH_WRITECR	40

#define		CH_PUSHC	41
#define		CH_PUSHI	42
#define		CH_PUSHD	43
#define		CH_PUSHVC	44
#define		CH_PUSHVI	45
#define		CH_PUSHVD	46
#define		CH_PUSHVM	47
#define		CH_PUSHM	48
#define		CH_PUSHCC	49
#define		CH_PUSHCI	50
#define		CH_PUSHCD	51
#define		CH_PUSHCM	52

#define		CH_POPC		53
#define		CH_POPI		54
#define		CH_POPD		55
#define		CH_POPVC	56
#define		CH_POPVI	57
#define		CH_POPVD	58
#define		CH_POPVM	59
#define		CH_POPM		60
#define		CH_POPX		61
#define		CH_POPZ		62

#endif /*CH_ASM_H_*/
