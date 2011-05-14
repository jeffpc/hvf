.text
	.align	4
.globl IOINT
	.type	IOINT, @function
IOINT:
	stmg	%r1,%r2,512

	l	%r1,0xb8
	larl	%r2,irb
	tsch	0(%r2)

	l	%r1,5(%r2)
	nill	%r1,0x04
	brc	7,4f		# device end

	l	%r1,5(%r2)
	nill	%r1,0x80
	brc	7,4f		# attention

	l	%r1,5(%r2)
	nill	%r1,0x03
	brc	7,4f		# ucheck or uexcept

4:
	la	%r1,1
	st	%r1,0x170
3:
	mvc	0x210(1,%r0),8(%r2)
	lmg	%r1,%r2,512
	lpswe	0x170

.data
irb:
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
