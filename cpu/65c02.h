// *******************************************************************************************
// *******************************************************************************************
//
//		File:		65C02.H
//		Date:		3rd September 2019
//		Purpose:	Additional functions for new 65C02 Opcodes.
//		Author:		Paul Robson (paul@robson.org.uk)
//
// *******************************************************************************************
// *******************************************************************************************

// *******************************************************************************************
//
//					Indirect without indexation.  (copied from indy)
//
// *******************************************************************************************

static void ind0() {
    uint16_t eahelp, eahelp2;
    eahelp = (uint16_t)read6502(pc++);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //zero-page wraparound
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
}


// *******************************************************************************************
//
//						(Absolute,Indexed) address mode for JMP
//
// *******************************************************************************************

static void ainx() { 		// absolute indexed branch
    uint16_t eahelp, eahelp2;
    eahelp = (uint16_t)read6502(pc) | (uint16_t)((uint16_t)read6502(pc+1) << 8);
    eahelp = (eahelp + (uint16_t)x) & 0xFFFF;
#if 0
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //replicate 6502 page-boundary wraparound bug
#else
    eahelp2 = eahelp + 1; // the 65c02 doesn't have the bug
#endif
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    pc += 2;
}

// *******************************************************************************************
//
//								Store zero to memory.
//
// *******************************************************************************************

static void stz() {
    putvalue(0);
}

// *******************************************************************************************
//
//								Unconditional Branch
//
// *******************************************************************************************

static void bra() {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
        else clockticks6502++;
}

// *******************************************************************************************
//
//									Push/Pull X and Y
//
// *******************************************************************************************

static void phx() {
    push8(x);
}

static void plx() {
    x = pull8();
   
    zerocalc(x);
    signcalc(x);
}

static void phy() {
    push8(y);
}

static void ply() {
    y = pull8();
  
    zerocalc(y);
    signcalc(y);
}

// *******************************************************************************************
//
//								TRB & TSB - Test and Change bits 
//
// *******************************************************************************************

static void tsb() {
    value = getvalue(); 							// Read memory
    result = (uint16_t)a & value;  					// calculate A & memory
    zerocalc(result); 								// Set Z flag from this.
    result = value | a; 							// Write back value read, A bits are set.
    putvalue(result);
}

static void trb() {
    value = getvalue(); 							// Read memory
    result = (uint16_t)a & value;  					// calculate A & memory
    zerocalc(result); 								// Set Z flag from this.
    result = value & (a ^ 0xFF); 					// Write back value read, A bits are clear.
    putvalue(result);
}

// *******************************************************************************************
//
//                                     Invoke Debugger
//
// *******************************************************************************************

static void dbg() {
    DEBUGBreakToDebugger();                          // Invoke debugger.
}
