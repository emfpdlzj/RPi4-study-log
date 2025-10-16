ASSIGN		START	0
.word: 정수 3바이트를 메모리에 저장함
.resw: word공간 (3바이트) 예약함
.resb: 바이트 예약함
.BYTE: 메모리에 1바이트값을 정의하거나 문자열/헥사 데이터를 저장
.RD/WD: 1바이트씩 입출력
.A Dirext, #A:Immdediate, A,x:Indexed, @A:Indirejct
MAIN		CLEAR A
		JSUB STINIT
		JSUB RDREC
		J INTOPOST .infix to postfix
AFTITP		JSUB WRREC1 .write postfix
		JSUB CALCU .calculate postfix
ENDCAL		JSUB POPW .calcu 결과 스택에서 pop
		STA BUFFER9 .calcu결과 저장용 버퍼		
		JSUB PRINTDEC .10진수로 결과 출력해줌. 
		J	HALT .프로그램 종료 

STACKP		RESW 	1
STACK		RESB	102 .1바이트 접근용 스택
STACK2		RESB	102 .3바이트 접근용 스택
READRES		RESB 101 .RDREC RES 
POSTFIX		RESB 101 .postfix 결과
LENGTH		RESB 101 .LENGTH OF READRES
INDEXP		RESW	1 .지금 읽고있는 길이 나타내는 인덱스
TMP		RESW	1 .읽은 값 저장해둠.
RINDEX		RESW	1 .읽은 값 저장해두는 길이 나타내는 인덱스
PRE1		RESW	1 .현재 우선순위
PRE2		RESW	1 .읽은값 우선순위
EOF		BYTE C'EOF'
ASEN		WORD 10       .ASCII CODE OF 'ENTER'
BUFFER		RESB 30	.rerec에서 쓰는 버퍼
MAX		WORD	1 .print에서 사용하는 max값
HALT		J	HALT .프로그램 종료

STINIT		CLEAR A .1바이트용 스택 초기화 
		LDA #STACK .a에 stack의 주소를 받아와서 stackp에 저장
		STA STACKP
		RSUB
PUSH		STCH @STACKP  .A값이 stack에 들어감.
		LDA STACKP
		ADD #1
		STA STACKP
		CLEAR A
		RSUB
POP		LDA STACKP
		COMP #STACK
		JEQ HERE .stack이 비어있으면 Here로 가서 종료함.
		SUB #1
		STA STACKP
		LDCH @STACKP .pop한 값이 A에 들어감.
HERE		RSUB

RDREC		LDX #0 .사용자의 입력을 읽어들이는 rdrec
		LDA #0
RLOOP		TD INPUT
		JEQ RLOOP
		RD INPUT .한 바이트 읽어와서 a레지스터 저장
		COMP    ASEN .TEST FOR END OF RECORD
		JEQ     EXIT    .EXIT LOOP IF EOR 
		STCH	READRES,X .store character in readres
		TIX 	MAXLEN	
		JLT	RLOOP	.Maxlen보다 작으면 반복.
EXIT		STX	LENGTH
		CLEAR A
		CLEAR X
		RSUB
INPUT		BYTE	0
MAXLEN		WORD	4096

INTOPOST	CLEAR X .infix to postfix
		CLEAR A
		STA INDEXP 
		STA RINDEX
		STA PRE1
		STA PRE2	
		JSUB STINIT .스택 초기화  .\
ILOOP		CLEAR A .ILOOP:intopost loop
		STCH	TMP .a,tmp 초기화
		LDX	INDEXP
		LDCH 	READRES,X .indexp번째 readres에서 값을 읽어서 A에 로드
		STCH	TMP 	.현재읽은값 저장
		CLEAR	A
		LDA	INDEXP
		ADD	#1
		STA	INDEXP
		CLEAR A
		LDCH TMP
		COMP	ASEN .TEST FOR END OF readres
		JEQ	AFTITP	.EXIT LOOP IF EOF
		LDCH	TMP .ascii *:42, +:43 -:45
		COMP #42
		JEQ ISOP      
		COMP #43
		JEQ ISOP  
		COMP #45
		JEQ ISOP     
		J       ISNUM .숫자일경우
ISOP		LDCH TMP	.연산자일경우
		STCH CURROP
		JSUB GETPRE       . PRE1 ← 현재 연산자 우선순위
		STA PRE1
WHILEOP		CLEAR A
		STA PRE2
		LDA STACKP
		COMP #STACK
		JEQ PUSHLAB       . 스택 비었으면 바로 push
		CLEAR A
		LDA STACKP 
		SUB #1
		STA TADDR .top 주소
		CLEAR A
		LDCH @TADDR .top주소가 가르키는 값
		STCH TOPOP .topop에 저장
		STCH TMP .tmp에 임시저장
		JSUB GETPRE
		STA PRE2
		COMP PRE1
		JLT PUSHLAB       .top 우선순위 낮으면 현재 연산자 push
		JSUB POP
		LDX RINDEX
		STCH POSTFIX,X
		CLEAR A
		LDA RINDEX
		ADD #1
		STA RINDEX
		J WHILEOP
ISNUM		LDCH TMP
		LDX RINDEX	.숫자일경우 postfix에 저장해주면됨.
		STCH POSTFIX,X
		CLEAR A
		LDA RINDEX
		ADD #1
		STA RINDEX
TESTLEN		CLEAR A
		LDA	INDEXP .a에는 읽어야할 문자열의 길이
		COMP	LENGTH
		JEQ 	CLEAN	.indexp가 a랑 같으면 읽기 종료하고 메인으로 돌아감.
		J	ILOOP .아니면 다시 iloop
PUSHLAB		LDCH	CURROP .push하는 label
		JSUB	PUSH
		J	TESTLEN	
CLEAN		LDA STACKP .스택에 남은값 모두정리
		COMP #STACK
		JEQ AFTITP
		LDA STACKP
		SUB #1
		STA TADDR
		LDCH @TADDR
		JSUB POP
		LDX RINDEX
		STCH POSTFIX,X
		LDA RINDEX
		ADD #1
		STA RINDEX
		J CLEAN
GETPRE		CLEAR 	A .우선순위 구하는 함수
		LDCH TMP
		COMP	#42
		JEQ	SET2
		COMP    #43
		JEQ	SET1
		COMP    #45
		JEQ	SET1
HERE2		RSUB
SET1		LDA	ONE .add, sub은 우선순위 set1
		J	HERE2
SET2		LDA	TWO .mul은 우선순위 set2
		J	HERE2

WRREC1		LDX	ZERO .postfix 출력하는 함수
WLOOP1		TD	OUTPUT
		JEQ	WLOOP1
		LDCH	POSTFIX,X .getcharacter from postfix
		WD	OUTPUT
		TIX	LENGTH
		JLT	WLOOP1
		LDA	#10
		WD	#1
		RSUB

CALCU		CLEAR X .Postfix 결과 계산하는 함수 
		CLEAR A
		STA	INDEXC .읽어들이는 for counter 변수
		STA	TMP2
		JSUB STINITW
CALOOP		CLEAR A .calcu loop
		STA	TMP2
		STA	OPRES
		LDX	INDEXC
		LDCH	POSTFIX,X
		STCH	TMP2
		CLEAR	A
		LDA	INDEXC
		ADD	#1
		STA	INDEXC
		CLEAR	A
		LDCH	TMP2.ascii *:42, +:43 -:45
		COMP #42
		JEQ ISOP2      
		COMP #43
		JEQ ISOP2 
		COMP #45
		JEQ ISOP2    
		J       ISNUM2 .숫자일경우
ISOP2		CLEAR A .연산자일경우
		JSUB POPW
		STA	OP1 .먼저 pop, 두번째 피연산자
		CLEAR A
		JSUB	POPW
		STA	OP2
		LDCH TMP2.ascii *:42, +:43 -:45
		COMP #42
		JEQ OPMUL    
		COMP #43
		JEQ OPADD
		COMP #45
		JEQ OPSUB
OPMUL		CLEAR A .연산자 곱하기일경우
		LDA	OP2
		MUL	OP1
		STA	OPRES
		JSUB	PUSHW
		J	TESTLEN2
OPADD		CLEAR A .연산자 더하기일경우
		LDA	OP2
		ADD	OP1
		STA	OPRES
		JSUB	PUSHW
		J	TESTLEN2
OPSUB		CLEAR A .연산자 빼기일경우
		LDA	OP2
		SUB	OP1
		STA	OPRES
		JSUB	PUSHW
		J	TESTLEN2
ISNUM2		CLEAR A .숫자일경우
		LDCH TMP2
		SUB #48
		MUL #1
		JSUB PUSHW
		J	TESTLEN2
TESTLEN2	CLEAR A .길이 비교
		LDA	INDEXC .a에는 읽어야할 문자열의 길이
		COMP	LENGTH
		JEQ 	ENDCAL	.indexp가 a랑 같으면 읽기 종료하고 메인으로 돌아감.
		J	CALOOP .아니면 다시 iloop

STINITW		CLEAR A	.3바이트용 스택 초기화 
		LDA #STACK2
		STA STACKP
		RSUB
PUSHW		STA @STACKP  .A값이 stack에 들어감.
		LDA STACKP
		ADD #3
		STA STACKP
		CLEAR A
		RSUB
POPW		LDA STACKP
		COMP #STACK2
		JEQ HERE3
		SUB #3
		STA STACKP
		LDA @STACKP .pop한 값이 A에 들어감.
HERE3		RSUB

PRINTDEC	STL     SAVERET     .10진수로 바꿔서 출력하는 함수
		LDA     BUFFER9     
		STA     WTEMP       .계산용 버퍼에 복사
		LDA     WTEMP
		COMP    #100
		JLT     TENS        .100 미만 → 10자리 처리
HUNDREDS	LDA     WTEMP	.100이상
		DIV     #100
		ADD     #48
		WD      #1          . 백의 자리 출력
		SUB     #48
		MUL     #100
		STA     TMP9
		LDA     WTEMP
		SUB     TMP9
		STA     WTEMP
TENS		LDA     WTEMP
		COMP    #10
		JLT     ONES        .10 미만 → 1자리만 처리
		DIV     #10
		ADD     #48
		WD      #1          .십의 자리 출력
		SUB     #48
		MUL     #10
		STA     TMP9
		LDA     WTEMP
		SUB     TMP9
		STA     WTEMP 
ONES		LDA     WTEMP
		ADD     #48
		WD      #1          .	일의 자리 출력
		LDA     #10
		WD      #1          .개행문자 출력, Print 참고..
		LDL     SAVERET
		RSUB

WTEMP		RESW	2 .print에서 출력용으로 사용하는 임시변수 write tmp
TMP9		RESW    1 .Temporary storage
BUFFER9		RESW    1 .Hex value to convert
SAVERET		RESW    1 .Return address storage
OPRES		RESW	1 .calcu에서 중간연산 결과 저장 변수
TMP2		RESW	1 .calcu에서 사용하는 Tmp
INDEXC		RESW	1 .읽어들이는 for counter 변수
OP1		RESW	1 
OP2		RESW	1
OUTPUT		BYTE	1
TWO		WORD 2
ONE		WORD 1
ZERO		WORD 0		
CURROP		RESW 1 .current op 저장용
TADDR		RESW 1 .top stack adress 저장용
TOPOP		RESW	1
		END ASSIGN  