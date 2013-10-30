const u32 stub_1800_1_512[] __attribute__ ((aligned(32))) =
{

//check_pvr_hi r4, 0x7001
    /*[0] 0x1800*/  0x7c9f42a6 //mfpvr   r4
    /*[1] 0x1804*/, 0x5484843e //rlwinm  r4,r4,16,16,31
    /*[2] 0x1808*/, 0x28047001 //cmplwi  r4,28673
//bne __real_start:
    /*[3] 0x180c*/, 0x408200b4 //bne-    0x18c0  0x408200f4 original

    /*[4] 0x1810*/, 0x7c79faa6 //mfl2cr  r3
    /*[5] 0x1814*/, 0x5463003e //rotlwi  r3,r3,0	rlwinm..?
    /*[6] 0x1818*/, 0x7c79fba6 //mtl2cr  r3
    /*[7] 0x181c*/, 0x7c0004ac //sync    
//spr_oris r3, l2cr, 0x0020		/* L2 global invalidate */
    /*[8] 0x1820*/, 0x7c79faa6 //mfl2cr  r3
    /*[9] 0x1824*/, 0x64630020 //oris    r3,r3,32
    /*[10] 0x1828*/, 0x7c79fba6 //mtl2cr  r3
//spr_check_bit r3, l2cr, 31, 0	/* L2IP */
//local_0:
    /*[11] 0x182c*/, 0x7c79faa6 //mfl2cr  r3
    /*[12] 0x1830*/, 0x546307fe //clrlwi  r3,r3,31

    /*[13] 0x1834*/, 0x2c030000 //cmpwi   r3,0
    /*[14] 0x1838*/, 0x4082fff4 //bne+    0x182c	local_0:
//spr_clear_bit r3, l2cr, 10		/* clear L2I */
    /*[15] 0x183c*/, 0x7c79faa6 //mfl2cr  r3
    /*[16] 0x1840*/, 0x546302d2 //rlwinm  r3,r3,0,11,9
    /*[17] 0x1844*/, 0x7c79fba6 //mtl2cr  r3

//spr_check_bit r3, l2cr, 31, 0	/* clear L2I */
//local_1:
    /*[18] 0x1848*/, 0x7c79faa6 //mfl2cr  r3
    /*[19] 0x184c*/, 0x546307fe //clrlwi  r3,r3,31
    /*[20] 0x1850*/, 0x2c030000 //cmpwi   r3,0
    /*[21] 0x1854*/, 0x4082fff4 //bne+    0x1848	local_1:

    /*[22] 0x1858*/, 0x48000004 //b       0x185c	start:	0x48000008 original

//original there is a 0x00000000 located here

//spr_clear_bit r3, hid5, 0		/* disable HID5 */
//start:
    /*[23] 0x185c*/, 0x7c70eaa6 //mfspr   r3,944	Ox3b0 hid5?
    /*[24] 0x1860*/, 0x5463007e //clrlwi  r3,r3,1
    /*[25] 0x1864*/, 0x7c70eba6 //mtspr   944,r3

    /*[26] 0x1868*/, 0x60000000 //nop
    /*[27] 0x186c*/, 0x7c0004ac //sync    
    /*[28] 0x1870*/, 0x60000000 //nop
    /*[29] 0x1874*/, 0x60000000 //nop
    /*[30] 0x1878*/, 0x60000000 //nop

//spr_oris r3, bcr, 0x1000
    /*[31] 0x187c*/, 0x7c75eaa6 //mfspr   r3,949
    /*[32] 0x1880*/, 0x64631000 //oris    r3,r3,4096
    /*[33] 0x1884*/, 0x7c75eba6 //mtspr   949,r3

    /*[34] 0x1888*/, 0x388000ff //li      r4,255
//delay loop?
//local_2:
    /*[35] 0x188c*/, 0x3884ffff //addi    r4,r4,-1
    /*[36] 0x1890*/, 0x2c040000 //cmpwi   r4,0
    /*[37] 0x1894*/, 0x4082fff8 //bne+    0x188c	local_2:

    /*[38] 0x1898*/, 0x60000000 //nop

//set_srr0_phys r3, __real_start
    /*[39] 0x189c*/, 0x3c600000 //lis     r3,0
    /*[40] 0x18a0*/, 0x606318c0 //ori     r3,r3,6336
    /*[41] 0x18a4*/, 0x5463007e //clrlwi  r3,r3,1
    /*[42] 0x18a8*/, 0x7c7a03a6 //mtsrr0  r3

    /*[43] 0x18ac*/, 0x38800000 //li      r4,0
    /*[44] 0x18b0*/, 0x7c9b03a6 //mtsrr1  r4
    /*[45] 0x18b4*/, 0x4c000064 //rfi
//.align 4
    /*[46] 0x18b8*/, 0x60000000 //nop
    /*[47] 0x18bc*/, 0x60000000 //nop

// in original code this starts at 0x01330200
// previous area is filled with 68 0x00 bytes

//__real_start:
//spr_set r4, hid0, 0x00110C64	/* DPM, NHR, ICFI, DCFI, DCFA, BTIC, and BHT */
    /*[48] 0x18c0*/, 0x3c800011 //lis     r4,17
    /*[49] 0x18c4*/, 0x60840c64 //ori     r4,r4,3172	0x0c64		   0x38840c64 original
    /*[50] 0x18c8*/, 0x7c90fba6 //mtspr   1008,r4		0x3f0 = hid0?
//msr_set r4, 0x2000				/* FP */
    /*[51] 0x18cc*/, 0x3c800000 //lis     r4,0
    /*[52] 0x18d0*/, 0x60842000 //ori     r4,r4,8192	0x2000		   0x38842000 original
    /*[53] 0x18d4*/, 0x7c800124 //mtmsr   r4
//spr_oris r4, hid4, 0x0200		/* enable SBE */
    /*[54] 0x18d8*/, 0x7c93faa6 //mfspr   r4,1011		 0x3f3 = hid4?
    /*[55] 0x18dc*/, 0x64840200 //oris    r4,r4,512	 0x548401ca original	
    /*[56] 0x18e0*/, 0x7c93fba6 //mtspr   1011,r4
//spr_ori r4, hid0, 0xC000		/* ICE, DCE */
    /*[57] 0x18e4*/, 0x7c90faa6 //mfspr   r4,1008		0x3f0 = hid0?  0x7c70faa6 original
    /*[58] 0x18e8*/, 0x6084c000 //ori     r4,r4,49152				   0x6054c000 original
    /*[59] 0x18ec*/, 0x7c90fba6 //mtspr   1008,r4
    /*[60] 0x18f0*/, 0x4c00012c //isync

//.irp b,0u,0l,1u,1l,2u,2l,3u,3l,4u,4l,5u,5l,6u,6l,7u,7l
    /*[61] 0x18f4*/, 0x38800000 //li      r4,0
    /*[62] 0x18f8*/, 0x7c9883a6 //mtdbatu 0,r4			mov to .. bat0u
    /*[63] 0x18fc*/, 0x7c9983a6 //mtdbatl 0,r4			mov to .. bat0l	 not in original?
    /*[64] 0x1900*/, 0x7c9a83a6 //mtdbatu 1,r4
    /*[65] 0x1904*/, 0x7c9b83a6 //mtdbatl 1,r4			not in original?
    /*[66] 0x1908*/, 0x7c9c83a6 //mtdbatu 2,r4
    /*[67] 0x190c*/, 0x7c9d83a6 //mtdbatl 2,r4
    /*[68] 0x1910*/, 0x7c9e83a6 //mtdbatu 3,r4
    /*[69] 0x1914*/, 0x7c9f83a6 //mtdbatl 3,r4
//mtsr sr\sr, r4
    /*[70] 0x1918*/, 0x7c988ba6 //mtspr   568,r4		move to spr 0x0238
    /*[71] 0x191c*/, 0x7c998ba6 //mtspr   569,r4		0x0239
    /*[72] 0x1920*/, 0x7c9a8ba6 //mtspr   570,r4
    /*[73] 0x1924*/, 0x7c9b8ba6 //mtspr   571,r4
    /*[74] 0x1928*/, 0x7c9c8ba6 //mtspr   572,r4
    /*[75] 0x192c*/, 0x7c9d8ba6 //mtspr   573,r4
    /*[76] 0x1930*/, 0x7c9e8ba6 //mtspr   574,r4
    /*[77] 0x1934*/, 0x7c9f8ba6 //mtspr   575,r4
//.irp b,0u,0l,1u,1l,2u,2l,3u,3l,4u,4l,5u,5l,6u,6l,7u,7l
    /*[78] 0x1938*/, 0x7c9083a6 //mtibatu 0,r4
    /*[79] 0x193c*/, 0x7c9183a6 //mtibatl 0,r4
    /*[80] 0x1940*/, 0x7c9283a6 //mtibatu 1,r4
    /*[81] 0x1944*/, 0x7c9383a6 //mtibatl 1,r4
    /*[82] 0x1948*/, 0x7c9483a6 //mtibatu 2,r4
    /*[83] 0x194c*/, 0x7c9583a6 //mtibatl 2,r4
    /*[84] 0x1950*/, 0x7c9683a6 //mtibatu 3,r4
    /*[85] 0x1954*/, 0x7c9783a6 //mtibatl 3,r4
//mtspr ibat\b, r4
    /*[86] 0x1958*/, 0x7c908ba6 //mtspr   560,r4
    /*[87] 0x195c*/, 0x7c918ba6 //mtspr   561,r4
    /*[88] 0x1960*/, 0x7c928ba6 //mtspr   562,r4
    /*[89] 0x1964*/, 0x7c938ba6 //mtspr   563,r4
    /*[90] 0x1968*/, 0x7c948ba6 //mtspr   564,r4
    /*[91] 0x196c*/, 0x7c958ba6 //mtspr   565,r4
    /*[92] 0x1970*/, 0x7c968ba6 //mtspr   566,r4
    /*[93] 0x1974*/, 0x7c978ba6 //mtspr   567,r4

    /*[94] 0x1978*/, 0x4c00012c //isync

//.irp sr,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
//			mtsr sr\sr, r4
    /*[95] 0x197c*/, 0x3c808000 //lis     r4,-32768
    /*[96] 0x1980*/, 0x60840000 //ori     r4,r4,0		original 0x38840000

    /*[97] 0x1984*/, 0x7c8001a4 //mtsr    0,r4			move to spr sr?
    /*[98] 0x1988*/, 0x7c8101a4 //mtsr    1,r4
    /*[99] 0x198c*/, 0x7c8201a4 //mtsr    2,r4
    /*[100] 0x1990*/, 0x7c8301a4 //mtsr    3,r4
    /*[101] 0x1994*/, 0x7c8401a4 //mtsr    4,r4
    /*[102] 0x1998*/, 0x7c8501a4 //mtsr    5,r4
    /*[103] 0x199c*/, 0x7c8601a4 //mtsr    6,r4
    /*[104] 0x19a0*/, 0x7c8701a4 //mtsr    7,r4
    /*[105] 0x19a4*/, 0x7c8801a4 //mtsr    8,r4
    /*[106] 0x19a8*/, 0x7c8901a4 //mtsr    9,r4
    /*[107] 0x19ac*/, 0x7c8a01a4 //mtsr    10,r4
    /*[108] 0x19b0*/, 0x7c8b01a4 //mtsr    11,r4
    /*[109] 0x19b4*/, 0x7c8c01a4 //mtsr    12,r4
    /*[110] 0x19b8*/, 0x7c8d01a4 //mtsr    13,r4
    /*[111] 0x19bc*/, 0x7c8e01a4 //mtsr    14,r4
    /*[112] 0x19c0*/, 0x7c8f01a4 //mtsr    15,r4

//set_bats r4, bat0l, 0x00000002		/* 0x00000000-0x10000000 : RW/RW Cached   */
    /*[113] 0x19c4*/, 0x3c800000 //lis     r4,0
    /*[114] 0x19c8*/, 0x60840002 //ori     r4,r4,2		original 0x38840002
    /*[115] 0x19cc*/, 0x7c9183a6 //mtibatl 0,r4			original not there
    /*[116] 0x19d0*/, 0x7c9983a6 //mtdbatl 0,r4			original not there
//set_bats r3, bat0u, 0x80001FFF		/* 0x80000000-0x90000000 :  1/ 1 :        */
    /*[117] 0x19d4*/, 0x3c608000 //lis     r3,-32768
    /*[118] 0x19d8*/, 0x60631fff //ori     r3,r3,8191	original 0x38631fff
    /*[119] 0x19dc*/, 0x7c7083a6 //mtibatu 0,r3			original 0x7c9983a6
    /*[120] 0x19e0*/, 0x7c7883a6 //mtdbatu 0,r3

    /*[121] 0x19e4*/, 0x4c00012c //isync
//set_bat r4, dbat1l, 0x0000002A		/* 0x00000000-0x10000000 : RW/RW Uncached */
    /*[122] 0x19e8*/, 0x3c800000 //lis     r4,0
    /*[123] 0x19ec*/, 0x6084002a //ori     r4,r4,42
    /*[124] 0x19f0*/, 0x7c9b83a6 //mtdbatl 1,r4
//set_bat r3, dbat1u, 0xC0001FFF		/* 0xC0000000-0xD0000000 :  1/ 1 : I/G    */
    /*[125] 0x19f4*/, 0x3c60c000 //lis     r3,-16384
    /*[126] 0x19f8*/, 0x60631fff //ori     r3,r3,8191
    /*[127] 0x19fc*/, 0x7c7a83a6 //mtdbatu 1,r3

    /*[128] 0x1a00*/, 0x4c00012c //isync
//set_bats r4, bat4l, 0x10000002		/* 0x10000000-0x20000000 : RW/RW Cached   */
    /*[29] 0x1a04*/, 0x3c801000 //lis     r4,4096
    /*[130] 0x1a08*/, 0x60840002 //ori     r4,r4,2
    /*[131] 0x1a0c*/, 0x7c918ba6 //mtspr   561,r4
    /*[132] 0x1a10*/, 0x7c998ba6 //mtspr   569,r4

//set_bats r3, bat4u, 0x90001FFF		/* 0x90000000-0xA0000000 :  1/ 1 :        */
    /*[133] 0x1a14*/, 0x3c609000 //lis     r3,-28672
    /*[134] 0x1a18*/, 0x60631fff //ori     r3,r3,8191
    /*[135] 0x1a1c*/, 0x7c708ba6 //mtspr   560,r3
    /*[136] 0x1a20*/, 0x7c788ba6 //mtspr   568,r3

    /*[137] 0x1a24*/, 0x4c00012c //isync
//set_bat r4, dbat5l, 0x1000002A		/* 0x10000000-0x20000000 : RW/RW Uncached */
    /*[138] 0x1a28*/, 0x3c801000 //lis     r4,4096
    /*[139] 0x1a2c*/, 0x6084002a //ori     r4,r4,42
    /*[140] 0x1a30*/, 0x7c9b8ba6 //mtspr   571,r4
//set_bat r3, dbat5u, 0xD0001FFF		/* 0xD0000000-0xE0000000 :  1/ 1 : I/G    */
    /*[141] 0x1a34*/, 0x3c60d000 //lis     r3,-12288
    /*[142] 0x1a38*/, 0x60631fff //ori     r3,r3,8191
    /*[143] 0x1a3c*/, 0x7c7a8ba6 //mtspr   570,r3

    /*[144] 0x1a40*/, 0x4c00012c //isync

    /*[145] 0x1a44*/, 0x38600000 //li      r3,0								original 0x3c600000
    /*[146] 0x1a48*/, 0x38800000 //li      r4,0
    /*[147] 0x1a4c*/, 0x908300f4 //stw     r4,244(r3)		BI2 0xf4


//set_srr0 r3, __KernelInit
//no instruction at 0x1a50 in megazig code so...?
    /*[148] 0x1a50*/, 0x3c608000 //lis r3 0x8000  									original 0x3c608133
    /*[149] 0x1a54*/,	0x60631800 //ori     r3,r3,6144  	/* 0x1800 */	original 0x60630400
    /*[150] 0x1a58*/, 0x7c7a03a6 //mtsrr0  r3

//so loading entry where original loaded kernel...
//this is from Maxternal I assume

//    write32((u32)(&[148]), 0x3c600000 | entry >> 16 ); //lis     r3,entry@h
//    write32((u32)(&[149]), 0x60630000 | (entry & 0xffff) ); //ori     r3,r3,entry@l
//    //write32(&[150], 0x7c7a03a6); //mtsrr0  r3

//msr_to_srr1_ori r4, 0x30			/* enable DR|IR */
    /*[151] 0x1a5c*/, 0x7c8000a6 //mfmsr   r4
    /*[152] 0x1a60*/, 0x60840030 //ori     r4,r4,48		0x30
    /*[153] 0x1a64*/, 0x7c9b03a6 //mtsrr1  r4
    /*[154] 0x1a68*/, 0x4c000064 //rfi
};
const u32 stub_1800_1_512_size = sizeof(stub_1800_1_512) / 4;
const u32 stub_1800_1_512_location = 0x100;
