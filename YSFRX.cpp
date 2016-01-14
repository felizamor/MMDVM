/*
 *   Copyright (C) 2009-2015 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "Config.h"
#include "Globals.h"
#include "YSFRX.h"
#include "Utils.h"

const unsigned int BUFFER_LENGTH = 200U;

const q15_t SCALING_FACTOR = 19505;      // Q15(0.60)

const uint32_t PLLMAX = 0x10000U;
const uint32_t PLLINC = PLLMAX / YSF_RADIO_SYMBOL_LENGTH;
const uint32_t INC    = PLLINC / 32U;

const uint8_t SYNC_SYMBOL_ERRS = 2U;
const uint8_t SYNC_BIT_ERRS    = 4U;

const unsigned int MAX_SYNC_FRAMES = 4U + 1U;

const uint8_t BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

const uint8_t INTERLEAVE_TABLE_RX[] = {
   0U, 40U,  80U, 120U, 160U,
   2U, 42U,  82U, 122U, 162U,
   4U, 44U,  84U, 124U, 164U,
   6U, 46U,  86U, 126U, 166U,
   8U, 48U,  88U, 128U, 168U,
  10U, 50U,  90U, 130U, 170U,
  12U, 52U,  92U, 132U, 172U,
  14U, 54U,  94U, 134U, 174U,
  16U, 56U,  96U, 136U, 176U,
  18U, 58U,  98U, 138U, 178U,
  20U, 60U, 100U, 140U, 180U,
  22U, 62U, 102U, 142U, 182U,
  24U, 64U, 104U, 144U, 184U,
  26U, 66U, 106U, 146U, 186U,
  28U, 68U, 108U, 148U, 188U,
  30U, 70U, 110U, 150U, 190U,
  32U, 72U, 112U, 152U, 192U,
  34U, 74U, 114U, 154U, 194U,
  36U, 76U, 116U, 156U, 196U,
  38U, 78U, 118U, 158U, 198U};

const uint8_t BRANCH_TABLE1[] = {0U, 0U, 0U, 0U, 1U, 1U, 1U, 1U};

const uint8_t BRANCH_TABLE2[] = {0U, 1U, 1U, 0U, 0U, 1U, 1U, 0U};

const uint32_t DECODING_TABLE_23127[] = {
	0x000000U, 0x000001U, 0x000002U, 0x000003U, 0x000004U, 0x000005U, 0x000006U, 0x000007U, 0x000008U, 0x000009U, 
	0x00000AU, 0x00000BU, 0x00000CU, 0x00000DU, 0x00000EU, 0x024020U, 0x000010U, 0x000011U, 0x000012U, 0x000013U, 
	0x000014U, 0x000015U, 0x000016U, 0x412000U, 0x000018U, 0x000019U, 0x00001AU, 0x180800U, 0x00001CU, 0x200300U, 
	0x048040U, 0x001480U, 0x000020U, 0x000021U, 0x000022U, 0x000023U, 0x000024U, 0x000025U, 0x000026U, 0x024008U, 
	0x000028U, 0x000029U, 0x00002AU, 0x024004U, 0x00002CU, 0x024002U, 0x024001U, 0x024000U, 0x000030U, 0x000031U, 
	0x000032U, 0x008180U, 0x000034U, 0x000C40U, 0x301000U, 0x0C0200U, 0x000038U, 0x043000U, 0x400600U, 0x210040U, 
	0x090080U, 0x508000U, 0x002900U, 0x024010U, 0x000040U, 0x000041U, 0x000042U, 0x000043U, 0x000044U, 0x000045U, 
	0x000046U, 0x280080U, 0x000048U, 0x000049U, 0x00004AU, 0x002500U, 0x00004CU, 0x111000U, 0x048010U, 0x400A00U, 
	0x000050U, 0x000051U, 0x000052U, 0x021200U, 0x000054U, 0x000C20U, 0x048008U, 0x104100U, 0x000058U, 0x404080U, 
	0x048004U, 0x210020U, 0x048002U, 0x0A2000U, 0x048000U, 0x048001U, 0x000060U, 0x000061U, 0x000062U, 0x540000U, 
	0x000064U, 0x000C10U, 0x010300U, 0x00B000U, 0x000068U, 0x088200U, 0x001880U, 0x210010U, 0x602000U, 0x040180U, 
	0x180400U, 0x024040U, 0x000070U, 0x000C04U, 0x086000U, 0x210008U, 0x000C01U, 0x000C00U, 0x420080U, 0x000C02U, 
	0x120100U, 0x210002U, 0x210001U, 0x210000U, 0x005200U, 0x000C08U, 0x048020U, 0x210004U, 0x000080U, 0x000081U, 
	0x000082U, 0x000083U, 0x000084U, 0x000085U, 0x000086U, 0x280040U, 0x000088U, 0x000089U, 0x00008AU, 0x050200U, 
	0x00008CU, 0x00A800U, 0x500100U, 0x001410U, 0x000090U, 0x000091U, 0x000092U, 0x008120U, 0x000094U, 0x160000U, 
	0x004A00U, 0x001408U, 0x000098U, 0x404040U, 0x222000U, 0x001404U, 0x090020U, 0x001402U, 0x001401U, 0x001400U, 
	0x0000A0U, 0x0000A1U, 0x0000A2U, 0x008110U, 0x0000A4U, 0x401200U, 0x042400U, 0x110800U, 0x0000A8U, 0x300400U, 
	0x001840U, 0x482000U, 0x090010U, 0x040140U, 0x208200U, 0x024080U, 0x0000B0U, 0x008102U, 0x008101U, 0x008100U, 
	0x090008U, 0x206000U, 0x420040U, 0x008104U, 0x090004U, 0x020A00U, 0x144000U, 0x008108U, 0x090000U, 0x090001U, 
	0x090002U, 0x001420U, 0x0000C0U, 0x0000C1U, 0x0000C2U, 0x280004U, 0x0000C4U, 0x280002U, 0x280001U, 0x280000U, 
	0x0000C8U, 0x404010U, 0x001820U, 0x128000U, 0x020600U, 0x040120U, 0x016000U, 0x280008U, 0x0000D0U, 0x404008U, 
	0x110400U, 0x042800U, 0x003100U, 0x018200U, 0x420020U, 0x280010U, 0x404001U, 0x404000U, 0x080300U, 0x404002U, 
	0x300800U, 0x404004U, 0x048080U, 0x001440U, 0x0000E0U, 0x032000U, 0x001808U, 0x004600U, 0x10C000U, 0x040108U, 
	0x420010U, 0x280020U, 0x001802U, 0x040104U, 0x001800U, 0x001801U, 0x040101U, 0x040100U, 0x001804U, 0x040102U, 
	0x240200U, 0x181000U, 0x420004U, 0x008140U, 0x420002U, 0x000C80U, 0x420000U, 0x420001U, 0x00A400U, 0x404020U, 
	0x001810U, 0x210080U, 0x090040U, 0x040110U, 0x420008U, 0x102200U, 0x000100U, 0x000101U, 0x000102U, 0x000103U, 
	0x000104U, 0x000105U, 0x000106U, 0x041800U, 0x000108U, 0x000109U, 0x00010AU, 0x002440U, 0x00010CU, 0x200210U, 
	0x500080U, 0x098000U, 0x000110U, 0x000111U, 0x000112U, 0x0080A0U, 0x000114U, 0x200208U, 0x0A0400U, 0x104040U, 
	0x000118U, 0x200204U, 0x015000U, 0x460000U, 0x200201U, 0x200200U, 0x002820U, 0x200202U, 0x000120U, 0x000121U, 
	0x000122U, 0x008090U, 0x000124U, 0x182000U, 0x010240U, 0x600400U, 0x000128U, 0x410800U, 0x2C0000U, 0x101200U, 
	0x009400U, 0x0400C0U, 0x002810U, 0x024100U, 0x000130U, 0x008082U, 0x008081U, 0x008080U, 0x444000U, 0x031000U, 
	0x002808U, 0x008084U, 0x120040U, 0x084400U, 0x002804U, 0x008088U, 0x002802U, 0x200220U, 0x002800U, 0x002801U, 
	0x000140U, 0x000141U, 0x000142U, 0x002408U, 0x000144U, 0x428000U, 0x010220U, 0x104010U, 0x000148U, 0x002402U, 
	0x002401U, 0x002400U, 0x084800U, 0x0400A0U, 0x221000U, 0x002404U, 0x000150U, 0x0D0000U, 0x600800U, 0x104004U, 
	0x003080U, 0x104002U, 0x104001U, 0x104000U, 0x120020U, 0x009800U, 0x080280U, 0x002410U, 0x410400U, 0x200240U, 
	0x048100U, 0x104008U, 0x000160U, 0x205000U, 0x010204U, 0x0A0800U, 0x010202U, 0x040088U, 0x010200U, 0x010201U, 
	0x120010U, 0x040084U, 0x40C000U, 0x002420U, 0x040081U, 0x040080U, 0x010208U, 0x040082U, 0x120008U, 0x402200U, 
	0x041400U, 0x0080C0U, 0x288000U, 0x000D00U, 0x010210U, 0x104020U, 0x120000U, 0x120001U, 0x120002U, 0x210100U, 
	0x120004U, 0x040090U, 0x002840U, 0x481000U, 0x000180U, 0x000181U, 0x000182U, 0x008030U, 0x000184U, 0x014400U, 
	0x500008U, 0x022200U, 0x000188U, 0x0A1000U, 0x500004U, 0x204800U, 0x500002U, 0x040060U, 0x500000U, 0x500001U, 
	0x000190U, 0x008022U, 0x008021U, 0x008020U, 0x003040U, 0x480800U, 0x250000U, 0x008024U, 0x040C00U, 0x112000U, 
	0x080240U, 0x008028U, 0x02C000U, 0x200280U, 0x500010U, 0x001500U, 0x0001A0U, 0x008012U, 0x008011U, 0x008010U, 
	0x220800U, 0x040048U, 0x085000U, 0x008014U, 0x006200U, 0x040044U, 0x030400U, 0x008018U, 0x040041U, 0x040040U, 
	0x500020U, 0x040042U, 0x008003U, 0x008002U, 0x008001U, 0x008000U, 0x100600U, 0x008006U, 0x008005U, 0x008004U, 
	0x601000U, 0x00800AU, 0x008009U, 0x008008U, 0x090100U, 0x040050U, 0x002880U, 0x00800CU, 0x0001C0U, 0x100A00U, 
	0x064000U, 0x411000U, 0x003010U, 0x040028U, 0x008C00U, 0x280100U, 0x218000U, 0x040024U, 0x080210U, 0x002480U, 
	0x040021U, 0x040020U, 0x500040U, 0x040022U, 0x003004U, 0x220400U, 0x080208U, 0x008060U, 0x003000U, 0x003001U, 
	0x003002U, 0x104080U, 0x080202U, 0x404100U, 0x080200U, 0x080201U, 0x003008U, 0x040030U, 0x080204U, 0x030800U, 
	0x480400U, 0x04000CU, 0x302000U, 0x008050U, 0x040009U, 0x040008U, 0x010280U, 0x04000AU, 0x040005U, 0x040004U, 
	0x001900U, 0x040006U, 0x040001U, 0x040000U, 0x040003U, 0x040002U, 0x014800U, 0x008042U, 0x008041U, 0x008040U, 
	0x003020U, 0x040018U, 0x420100U, 0x008044U, 0x120080U, 0x040014U, 0x080220U, 0x008048U, 0x040011U, 0x040010U, 
	0x204400U, 0x040012U, 0x000200U, 0x000201U, 0x000202U, 0x000203U, 0x000204U, 0x000205U, 0x000206U, 0x108400U, 
	0x000208U, 0x000209U, 0x00020AU, 0x050080U, 0x00020CU, 0x200110U, 0x083000U, 0x400840U, 0x000210U, 0x000211U, 
	0x000212U, 0x021040U, 0x000214U, 0x200108U, 0x004880U, 0x0C0020U, 0x000218U, 0x200104U, 0x400420U, 0x00E000U, 
	0x200101U, 0x200100U, 0x130000U, 0x200102U, 0x000220U, 0x000221U, 0x000222U, 0x202800U, 0x000224U, 0x401080U, 
	0x010140U, 0x0C0010U, 0x000228U, 0x088040U, 0x400410U, 0x101100U, 0x140800U, 0x012400U, 0x208080U, 0x024200U, 
	0x000230U, 0x114000U, 0x400408U, 0x0C0004U, 0x02A000U, 0x0C0002U, 0x0C0001U, 0x0C0000U, 0x400402U, 0x020880U, 
	0x400400U, 0x400401U, 0x005040U, 0x200120U, 0x400404U, 0x0C0008U, 0x000240U, 0x000241U, 0x000242U, 0x021010U, 
	0x000244U, 0x046000U, 0x010120U, 0x400808U, 0x000248U, 0x088020U, 0x304000U, 0x400804U, 0x020480U, 0x400802U, 
	0x400801U, 0x400800U, 0x000250U, 0x021002U, 0x021001U, 0x021000U, 0x580000U, 0x018080U, 0x202400U, 0x021004U, 
	0x012800U, 0x140400U, 0x080180U, 0x021008U, 0x005020U, 0x200140U, 0x048200U, 0x400810U, 0x000260U, 0x088008U, 
	0x010104U, 0x004480U, 0x010102U, 0x320000U, 0x010100U, 0x010101U, 0x088001U, 0x088000U, 0x062000U, 0x088002U, 
	0x005010U, 0x088004U, 0x010108U, 0x400820U, 0x240080U, 0x402100U, 0x108800U, 0x021020U, 0x005008U, 0x000E00U, 
	0x010110U, 0x0C0040U, 0x005004U, 0x088010U, 0x400440U, 0x210200U, 0x005000U, 0x005001U, 0x005002U, 0x102080U, 
	0x000280U, 0x000281U, 0x000282U, 0x050008U, 0x000284U, 0x401020U, 0x004810U, 0x022100U, 0x000288U, 0x050002U, 
	0x050001U, 0x050000U, 0x020440U, 0x184000U, 0x208020U, 0x050004U, 0x000290U, 0x082400U, 0x004804U, 0x700000U, 
	0x004802U, 0x018040U, 0x004800U, 0x004801U, 0x109000U, 0x020820U, 0x080140U, 0x050010U, 0x442000U, 0x200180U, 
	0x004808U, 0x001600U, 0x0002A0U, 0x401004U, 0x1A0000U, 0x004440U, 0x401001U, 0x401000U, 0x208008U, 0x401002U, 
	0x006100U, 0x020810U, 0x208004U, 0x050020U, 0x208002U, 0x401008U, 0x208000U, 0x208001U, 0x240040U, 0x020808U, 
	0x013000U, 0x008300U, 0x100500U, 0x401010U, 0x004820U, 0x0C0080U, 0x020801U, 0x020800U, 0x400480U, 0x020802U, 
	0x090200U, 0x020804U, 0x208010U, 0x102040U, 0x0002C0U, 0x100900U, 0x40A000U, 0x004420U, 0x020408U, 0x018010U, 
	0x141000U, 0x280200U, 0x020404U, 0x203000U, 0x080110U, 0x050040U, 0x020400U, 0x020401U, 0x020402U, 0x400880U, 
	0x240020U, 0x018004U, 0x080108U, 0x021080U, 0x018001U, 0x018000U, 0x004840U, 0x018002U, 0x080102U, 0x404200U, 
	0x080100U, 0x080101U, 0x020410U, 0x018008U, 0x080104U, 0x102020U, 0x240010U, 0x004402U, 0x004401U, 0x004400U, 
	0x082800U, 0x401040U, 0x010180U, 0x004404U, 0x510000U, 0x088080U, 0x001A00U, 0x004408U, 0x020420U, 0x040300U, 
	0x208040U, 0x102010U, 0x240000U, 0x240001U, 0x240002U, 0x004410U, 0x240004U, 0x018020U, 0x420200U, 0x102008U, 
	0x240008U, 0x020840U, 0x080120U, 0x102004U, 0x005080U, 0x102002U, 0x102001U, 0x102000U, 0x000300U, 0x000301U, 
	0x000302U, 0x484000U, 0x000304U, 0x200018U, 0x010060U, 0x022080U, 0x000308U, 0x200014U, 0x028800U, 0x101020U, 
	0x200011U, 0x200010U, 0x044400U, 0x200012U, 0x000310U, 0x20000CU, 0x142000U, 0x010C00U, 0x200009U, 0x200008U, 
	0x409000U, 0x20000AU, 0x200005U, 0x200004U, 0x0800C0U, 0x200006U, 0x200001U, 0x200000U, 0x200003U, 0x200002U, 
	0x000320U, 0x060400U, 0x010044U, 0x101008U, 0x010042U, 0x00C800U, 0x010040U, 0x010041U, 0x006080U, 0x101002U, 
	0x101001U, 0x101000U, 0x4A0000U, 0x200030U, 0x010048U, 0x101004U, 0x081800U, 0x402040U, 0x224000U, 0x008280U, 
	0x100480U, 0x200028U, 0x010050U, 0x0C0100U, 0x058000U, 0x200024U, 0x400500U, 0x101010U, 0x200021U, 0x200020U, 
	0x002A00U, 0x200022U, 0x000340U, 0x100880U, 0x010024U, 0x248000U, 0x010022U, 0x081400U, 0x010020U, 0x010021U, 
	0x441000U, 0x034000U, 0x080090U, 0x002600U, 0x10A000U, 0x200050U, 0x010028U, 0x400900U, 0x00C400U, 0x402020U, 
	0x080088U, 0x021100U, 0x060800U, 0x200048U, 0x010030U, 0x104200U, 0x080082U, 0x200044U, 0x080080U, 0x080081U, 
	0x200041U, 0x200040U, 0x080084U, 0x200042U, 0x010006U, 0x402010U, 0x010004U, 0x010005U, 0x010002U, 0x010003U, 
	0x010000U, 0x010001U, 0x200C00U, 0x088100U, 0x01000CU, 0x101040U, 0x01000AU, 0x040280U, 0x010008U, 0x010009U, 
	0x402001U, 0x402000U, 0x010014U, 0x402002U, 0x010012U, 0x402004U, 0x010010U, 0x010011U, 0x120200U, 0x402008U, 
	0x0800A0U, 0x044800U, 0x005100U, 0x200060U, 0x010018U, 0x028400U, 0x000380U, 0x100840U, 0x201400U, 0x022004U, 
	0x0C8000U, 0x022002U, 0x022001U, 0x022000U, 0x006020U, 0x408400U, 0x080050U, 0x050100U, 0x011800U, 0x200090U, 
	0x500200U, 0x022008U, 0x430000U, 0x045000U, 0x080048U, 0x008220U, 0x100420U, 0x200088U, 0x004900U, 0x022010U, 
	0x080042U, 0x200084U, 0x080040U, 0x080041U, 0x200081U, 0x200080U, 0x080044U, 0x200082U, 0x006008U, 0x290000U, 
	0x440800U, 0x008210U, 0x100410U, 0x401100U, 0x0100C0U, 0x022020U, 0x006000U, 0x006001U, 0x006002U, 0x101080U, 
	0x006004U, 0x040240U, 0x208100U, 0x080C00U, 0x100404U, 0x008202U, 0x008201U, 0x008200U, 0x100400U, 0x100401U, 
	0x100402U, 0x008204U, 0x006010U, 0x020900U, 0x080060U, 0x008208U, 0x100408U, 0x2000A0U, 0x061000U, 0x414000U, 
	0x100801U, 0x100800U, 0x080018U, 0x100802U, 0x604000U, 0x100804U, 0x0100A0U, 0x022040U, 0x080012U, 0x100808U, 
	0x080010U, 0x080011U, 0x020500U, 0x040220U, 0x080014U, 0x00D000U, 0x08000AU, 0x100810U, 0x080008U, 0x080009U, 
	0x003200U, 0x018100U, 0x08000CU, 0x440400U, 0x080002U, 0x080003U, 0x080000U, 0x080001U, 0x080006U, 0x2000C0U, 
	0x080004U, 0x080005U, 0x029000U, 0x100820U, 0x010084U, 0x004500U, 0x010082U, 0x040208U, 0x010080U, 0x010081U, 
	0x006040U, 0x040204U, 0x080030U, 0x620000U, 0x040201U, 0x040200U, 0x010088U, 0x040202U, 0x240100U, 0x402080U, 
	0x080028U, 0x008240U, 0x100440U, 0x0A4000U, 0x010090U, 0x201800U, 0x080022U, 0x011400U, 0x080020U, 0x080021U, 
	0x408800U, 0x040210U, 0x080024U, 0x102100U, 0x000400U, 0x000401U, 0x000402U, 0x000403U, 0x000404U, 0x000405U, 
	0x000406U, 0x108200U, 0x000408U, 0x000409U, 0x00040AU, 0x002140U, 0x00040CU, 0x4C0000U, 0x210800U, 0x001090U, 
	0x000410U, 0x000411U, 0x000412U, 0x244000U, 0x000414U, 0x000860U, 0x0A0100U, 0x001088U, 0x000418U, 0x038000U, 
	0x400220U, 0x001084U, 0x106000U, 0x001082U, 0x001081U, 0x001080U, 0x000420U, 0x000421U, 0x000422U, 0x091000U, 
	0x000424U, 0x000850U, 0x042080U, 0x600100U, 0x000428U, 0x300080U, 0x400210U, 0x048800U, 0x009100U, 0x012200U, 
	0x180040U, 0x024400U, 0x000430U, 0x000844U, 0x400208U, 0x122000U, 0x000841U, 0x000840U, 0x01C000U, 0x000842U, 
	0x400202U, 0x084100U, 0x400200U, 0x400201U, 0x260000U, 0x000848U, 0x400204U, 0x0010A0U, 0x000440U, 0x000441U, 
	0x000442U, 0x002108U, 0x000444U, 0x000830U, 0x405000U, 0x070000U, 0x000448U, 0x002102U, 0x002101U, 0x002100U, 
	0x020280U, 0x20C000U, 0x180020U, 0x002104U, 0x000450U, 0x000824U, 0x110080U, 0x488000U, 0x000821U, 0x000820U, 
	0x202200U, 0x000822U, 0x281000U, 0x140200U, 0x024800U, 0x002110U, 0x410100U, 0x000828U, 0x048400U, 0x0010C0U, 
	0x000460U, 0x000814U, 0x228000U, 0x004280U, 0x000811U, 0x000810U, 0x180008U, 0x000812U, 0x054000U, 0x421000U, 
	0x180004U, 0x002120U, 0x180002U, 0x000818U, 0x180000U, 0x180001U, 0x000805U, 0x000804U, 0x041100U, 0x000806U, 
	0x000801U, 0x000800U, 0x000803U, 0x000802U, 0x00A080U, 0x00080CU, 0x400240U, 0x210400U, 0x000809U, 0x000808U, 
	0x180010U, 0x00080AU, 0x000480U, 0x000481U, 0x000482U, 0x420800U, 0x000484U, 0x014100U, 0x042020U, 0x001018U, 
	0x000488U, 0x300020U, 0x08C000U, 0x001014U, 0x020240U, 0x001012U, 0x001011U, 0x001010U, 0x000490U, 0x082200U, 
	0x110040U, 0x00100CU, 0x608000U, 0x00100AU, 0x001009U, 0x001008U, 0x040900U, 0x001006U, 0x001005U, 0x001004U, 
	0x001003U, 0x001002U, 0x001001U, 0x001000U, 0x0004A0U, 0x300008U, 0x042004U, 0x004240U, 0x042002U, 0x0A8000U, 
	0x042000U, 0x042001U, 0x300001U, 0x300000U, 0x030100U, 0x300002U, 0x404800U, 0x300004U, 0x042008U, 0x001030U, 
	0x025000U, 0x450000U, 0x280800U, 0x008500U, 0x100300U, 0x0008C0U, 0x042010U, 0x001028U, 0x00A040U, 0x300010U, 
	0x400280U, 0x001024U, 0x090400U, 0x001022U, 0x001021U, 0x001020U, 0x0004C0U, 0x049000U, 0x110010U, 0x004220U, 
	0x020208U, 0x502000U, 0x008900U, 0x280400U, 0x020204U, 0x090800U, 0x640000U, 0x002180U, 0x020200U, 0x020201U, 
	0x020202U, 0x001050U, 0x110002U, 0x220100U, 0x110000U, 0x110001U, 0x0C4000U, 0x0008A0U, 0x110004U, 0x001048U, 
	0x00A020U, 0x404400U, 0x110008U, 0x001044U, 0x020210U, 0x001042U, 0x001041U, 0x001040U, 0x480100U, 0x004202U, 
	0x004201U, 0x004200U, 0x211000U, 0x000890U, 0x042040U, 0x004204U, 0x00A010U, 0x300040U, 0x001C00U, 0x004208U, 
	0x020220U, 0x040500U, 0x180080U, 0x418000U, 0x00A008U, 0x000884U, 0x110020U, 0x004210U, 0x000881U, 0x000880U, 
	0x420400U, 0x000882U, 0x00A000U, 0x00A001U, 0x00A002U, 0x0E0000U, 0x00A004U, 0x000888U, 0x204100U, 0x001060U, 
	0x000500U, 0x000501U, 0x000502U, 0x002048U, 0x000504U, 0x014080U, 0x0A0010U, 0x600020U, 0x000508U, 0x002042U, 
	0x002041U, 0x002040U, 0x009020U, 0x120800U, 0x044200U, 0x002044U, 0x000510U, 0x501000U, 0x0A0004U, 0x010A00U, 
	0x0A0002U, 0x04A000U, 0x0A0000U, 0x0A0001U, 0x040880U, 0x084020U, 0x308000U, 0x002050U, 0x410040U, 0x200600U, 
	0x0A0008U, 0x001180U, 0x000520U, 0x060200U, 0x104800U, 0x600004U, 0x009008U, 0x600002U, 0x600001U, 0x600000U, 
	0x009004U, 0x084010U, 0x030080U, 0x002060U, 0x009000U, 0x009001U, 0x009002U, 0x600008U, 0x212000U, 0x084008U, 
	0x041040U, 0x008480U, 0x100280U, 0x000940U, 0x0A0020U, 0x600010U, 0x084001U, 0x084000U, 0x400300U, 0x084002U, 
	0x009010U, 0x084004U, 0x002C00U, 0x150000U, 0x000540U, 0x00200AU, 0x002009U, 0x002008U, 0x340000U, 0x081200U, 
	0x008880U, 0x00200CU, 0x002003U, 0x002002U, 0x002001U, 0x002000U, 0x410010U, 0x002006U, 0x002005U, 0x002004U, 
	0x00C200U, 0x220080U, 0x041020U, 0x002018U, 0x410008U, 0x000920U, 0x0A0040U, 0x104400U, 0x410004U, 0x002012U, 
	0x002011U, 0x002010U, 0x410000U, 0x410001U, 0x410002U, 0x002014U, 0x480080U, 0x118000U, 0x041010U, 0x002028U, 
	0x026000U, 0x000910U, 0x010600U, 0x600040U, 0x200A00U, 0x002022U, 0x002021U, 0x002020U, 0x009040U, 0x040480U, 
	0x180100U, 0x002024U, 0x041002U, 0x000904U, 0x041000U, 0x041001U, 0x000901U, 0x000900U, 0x041004U, 0x000902U, 
	0x120400U, 0x084040U, 0x041008U, 0x002030U, 0x410020U, 0x000908U, 0x204080U, 0x028200U, 0x000580U, 0x014004U, 
	0x201200U, 0x1C0000U, 0x014001U, 0x014000U, 0x008840U, 0x014002U, 0x040810U, 0x408200U, 0x030020U, 0x0020C0U, 
	0x282000U, 0x014008U, 0x500400U, 0x001110U, 0x040808U, 0x220040U, 0x406000U, 0x008420U, 0x100220U, 0x014010U, 
	0x0A0080U, 0x001108U, 0x040800U, 0x040801U, 0x040802U, 0x001104U, 0x040804U, 0x001102U, 0x001101U, 0x001100U, 
	0x480040U, 0x003800U, 0x030008U, 0x008410U, 0x100210U, 0x014020U, 0x042100U, 0x600080U, 0x030002U, 0x300100U, 
	0x030000U, 0x030001U, 0x009080U, 0x040440U, 0x030004U, 0x080A00U, 0x100204U, 0x008402U, 0x008401U, 0x008400U, 
	0x100200U, 0x100201U, 0x100202U, 0x008404U, 0x040820U, 0x084080U, 0x030010U, 0x008408U, 0x100208U, 0x422000U, 
	0x204040U, 0x001120U, 0x480020U, 0x220010U, 0x008804U, 0x002088U, 0x008802U, 0x014040U, 0x008800U, 0x008801U, 
	0x105000U, 0x002082U, 0x002081U, 0x002080U, 0x020300U, 0x040420U, 0x008808U, 0x002084U, 0x220001U, 0x220000U, 
	0x110100U, 0x220002U, 0x003400U, 0x220004U, 0x008810U, 0x440200U, 0x040840U, 0x220008U, 0x080600U, 0x002090U, 
	0x410080U, 0x188000U, 0x204020U, 0x001140U, 0x480000U, 0x480001U, 0x480002U, 0x004300U, 0x480004U, 0x040408U, 
	0x008820U, 0x121000U, 0x480008U, 0x040404U, 0x030040U, 0x0020A0U, 0x040401U, 0x040400U, 0x204010U, 0x040402U, 
	0x480010U, 0x220020U, 0x041080U, 0x008440U, 0x100240U, 0x000980U, 0x204008U, 0x092000U, 0x00A100U, 0x011200U, 
	0x204004U, 0x500800U, 0x204002U, 0x040410U, 0x204000U, 0x204001U, 0x000600U, 0x000601U, 0x000602U, 0x108004U, 
	0x000604U, 0x108002U, 0x108001U, 0x108000U, 0x000608U, 0x005800U, 0x400030U, 0x2A0000U, 0x0200C0U, 0x012020U, 
	0x044100U, 0x108008U, 0x000610U, 0x082080U, 0x400028U, 0x010900U, 0x051000U, 0x424000U, 0x202040U, 0x108010U, 
	0x400022U, 0x140040U, 0x400020U, 0x400021U, 0x088800U, 0x200500U, 0x400024U, 0x001280U, 0x000620U, 0x060100U, 
	0x400018U, 0x0040C0U, 0x284000U, 0x012008U, 0x021800U, 0x108020U, 0x400012U, 0x012004U, 0x400010U, 0x400011U, 
	0x012001U, 0x012000U, 0x400014U, 0x012002U, 0x40000AU, 0x209000U, 0x400008U, 0x400009U, 0x100180U, 0x000A40U, 
	0x40000CU, 0x0C0400U, 0x400002U, 0x400003U, 0x400000U, 0x400001U, 0x400006U, 0x012010U, 0x400004U, 0x400005U, 
	0x000640U, 0x610000U, 0x0C0800U, 0x0040A0U, 0x020088U, 0x081100U, 0x202010U, 0x108040U, 0x020084U, 0x140010U, 
	0x019000U, 0x002300U, 0x020080U, 0x020081U, 0x020082U, 0x400C00U, 0x00C100U, 0x140008U, 0x202004U, 0x021400U, 
	0x202002U, 0x000A20U, 0x202000U, 0x202001U, 0x140001U, 0x140000U, 0x400060U, 0x140002U, 0x020090U, 0x140004U, 
	0x202008U, 0x094000U, 0x103000U, 0x004082U, 0x004081U, 0x004080U, 0x448000U, 0x000A10U, 0x010500U, 0x004084U, 
	0x200900U, 0x088400U, 0x400050U, 0x004088U, 0x0200A0U, 0x012040U, 0x180200U, 0x241000U, 0x0B0000U, 0x000A04U, 
	0x400048U, 0x004090U, 0x000A01U, 0x000A00U, 0x202020U, 0x000A02U, 0x400042U, 0x140020U, 0x400040U, 0x400041U, 
	0x005400U, 0x000A08U, 0x400044U, 0x028100U, 0x000680U, 0x082010U, 0x201100U, 0x004060U, 0x020048U, 0x240800U, 
	0x490000U, 0x108080U, 0x020044U, 0x408100U, 0x102800U, 0x050400U, 0x020040U, 0x020041U, 0x020042U, 0x001210U, 
	0x082001U, 0x082000U, 0x068000U, 0x082002U, 0x100120U, 0x082004U, 0x004C00U, 0x001208U, 0x214000U, 0x082008U, 
	0x4000A0U, 0x001204U, 0x020050U, 0x001202U, 0x001201U, 0x001200U, 0x018800U, 0x004042U, 0x004041U, 0x004040U, 
	0x100110U, 0x401400U, 0x042200U, 0x004044U, 0x0C1000U, 0x300200U, 0x400090U, 0x004048U, 0x020060U, 0x012080U, 
	0x208400U, 0x080900U, 0x100104U, 0x082020U, 0x400088U, 0x004050U, 0x100100U, 0x100101U, 0x100102U, 0x230000U, 
	0x400082U, 0x020C00U, 0x400080U, 0x400081U, 0x100108U, 0x04C000U, 0x400084U, 0x001220U, 0x02000CU, 0x004022U, 
	0x004021U, 0x004020U, 0x020008U, 0x020009U, 0x02000AU, 0x004024U, 0x020004U, 0x020005U, 0x020006U, 0x004028U, 
	0x020000U, 0x020001U, 0x020002U, 0x020003U, 0x401800U, 0x082040U, 0x110200U, 0x004030U, 0x020018U, 0x018400U, 
	0x202080U, 0x440100U, 0x020014U, 0x140080U, 0x080500U, 0x208800U, 0x020010U, 0x020011U, 0x020012U, 0x001240U, 
	0x004003U, 0x004002U, 0x004001U, 0x004000U, 0x020028U, 0x004006U, 0x004005U, 0x004004U, 0x020024U, 0x00400AU, 
	0x004009U, 0x004008U, 0x020020U, 0x020021U, 0x020022U, 0x00400CU, 0x240400U, 0x004012U, 0x004011U, 0x004010U, 
	0x100140U, 0x000A80U, 0x089000U, 0x004014U, 0x00A200U, 0x011100U, 0x4000C0U, 0x004018U, 0x020030U, 0x680000U, 
	0x050800U, 0x102400U, 0x000700U, 0x060020U, 0x201080U, 0x010810U, 0x402800U, 0x081040U, 0x044008U, 0x108100U, 
	0x190000U, 0x408080U, 0x044004U, 0x002240U, 0x044002U, 0x200410U, 0x044000U, 0x044001U, 0x00C040U, 0x010802U, 
	0x010801U, 0x010800U, 0x1000A0U, 0x200408U, 0x0A0200U, 0x010804U, 0x023000U, 0x200404U, 0x400120U, 0x010808U, 
	0x200401U, 0x200400U, 0x044010U, 0x200402U, 0x060001U, 0x060000U, 0x08A000U, 0x060002U, 0x100090U, 0x060004U, 
	0x010440U, 0x600200U, 0x200840U, 0x060008U, 0x400110U, 0x101400U, 0x009200U, 0x012100U, 0x044020U, 0x080880U, 
	0x100084U, 0x060010U, 0x400108U, 0x010820U, 0x100080U, 0x100081U, 0x100082U, 0x007000U, 0x400102U, 0x084200U, 
	0x400100U, 0x400101U, 0x100088U, 0x200420U, 0x400104U, 0x028040U, 0x00C010U, 0x081004U, 0x520000U, 0x002208U, 
	0x081001U, 0x081000U, 0x010420U, 0x081002U, 0x200820U, 0x002202U, 0x002201U, 0x002200U, 0x020180U, 0x081008U, 
	0x044040U, 0x002204U, 0x00C000U, 0x00C001U, 0x00C002U, 0x010840U, 0x00C004U, 0x081010U, 0x202100U, 0x440080U, 
	0x00C008U, 0x140100U, 0x080480U, 0x002210U, 0x410200U, 0x200440U, 0x101800U, 0x028020U, 0x200808U, 0x060040U, 
	0x010404U, 0x004180U, 0x010402U, 0x081020U, 0x010400U, 0x010401U, 0x200800U, 0x200801U, 0x200802U, 0x002220U, 
	0x200804U, 0x504000U, 0x010408U, 0x028010U, 0x00C020U, 0x402400U, 0x041200U, 0x380000U, 0x1000C0U, 0x000B00U, 
	0x010410U, 0x028008U, 0x200810U, 0x011080U, 0x400140U, 0x028004U, 0x0C2000U, 0x028002U, 0x028001U, 0x028000U, 
	0x201002U, 0x408008U, 0x201000U, 0x201001U, 0x100030U, 0x014200U, 0x201004U, 0x022400U, 0x408001U, 0x408000U, 
	0x201008U, 0x408002U, 0x020140U, 0x408004U, 0x044080U, 0x080820U, 0x100024U, 0x082100U, 0x201010U, 0x010880U, 
	0x100020U, 0x100021U, 0x100022U, 0x440040U, 0x040A00U, 0x408010U, 0x080440U, 0x124000U, 0x100028U, 0x200480U, 
	0x01A000U, 0x001300U, 0x100014U, 0x060080U, 0x201020U, 0x004140U, 0x100010U, 0x100011U, 0x100012U, 0x080808U, 
	0x006400U, 0x408020U, 0x030200U, 0x080804U, 0x100018U, 0x080802U, 0x080801U, 0x080800U, 0x100004U, 0x100005U, 
	0x100006U, 0x008600U, 0x100000U, 0x100001U, 0x100002U, 0x100003U, 0x10000CU, 0x011040U, 0x400180U, 0x242000U, 
	0x100008U, 0x100009U, 0x10000AU, 0x080810U, 0x052000U, 0x100C00U, 0x201040U, 0x004120U, 0x020108U, 0x081080U, 
	0x008A00U, 0x440010U, 0x020104U, 0x408040U, 0x080410U, 0x002280U, 0x020100U, 0x020101U, 0x020102U, 0x310000U, 
	0x00C080U, 0x220200U, 0x080408U, 0x440004U, 0x100060U, 0x440002U, 0x440001U, 0x440000U, 0x080402U, 0x011020U, 
	0x080400U, 0x080401U, 0x020110U, 0x006800U, 0x080404U, 0x440008U, 0x480200U, 0x004102U, 0x004101U, 0x004100U, 
	0x100050U, 0x20A000U, 0x010480U, 0x004104U, 0x200880U, 0x011010U, 0x148000U, 0x004108U, 0x020120U, 0x040600U, 
	0x403000U, 0x080840U, 0x100044U, 0x011008U, 0x022800U, 0x004110U, 0x100040U, 0x100041U, 0x100042U, 0x440020U, 
	0x011001U, 0x011000U, 0x080420U, 0x011002U, 0x100048U, 0x011004U, 0x204200U, 0x028080U};

static const unsigned short CCITT_TABLE[] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0};

CYSFRX::CYSFRX() :
m_pll(0U),
m_prev(false),
m_state(YSFRXS_NONE),
m_bitBuffer(0x00U),
m_symbols(),
m_outBuffer(),
m_buffer(NULL),
m_bufferPtr(0U),
m_symbolPtr(0U),
m_lostCount(0U),
m_centre(0),
m_threshold(0),
m_metrics1(),
m_metrics2(),
m_oldMetrics(NULL),
m_newMetrics(NULL),
m_decisions(),
m_dp(NULL)
{
  m_buffer = m_outBuffer + 1U;
}

void CYSFRX::reset()
{
  m_pll       = 0U;
  m_prev      = false;
  m_state     = YSFRXS_NONE;
  m_bitBuffer = 0x00U;
  m_bufferPtr = 0U;
  m_symbolPtr = 0U;
  m_lostCount = 0U;
  m_centre    = 0;
  m_threshold = 0;
}

void CYSFRX::samples(const q15_t* samples, uint8_t length)
{
  for (uint16_t i = 0U; i < length; i++) {
    bool bit = samples[i] < 0;

    if (bit != m_prev) {
      if (m_pll < (PLLMAX / 2U))
        m_pll += INC;
      else
        m_pll -= INC;
    }

    m_prev = bit;

    m_pll += PLLINC;

    if (m_pll >= PLLMAX) {
      m_pll -= PLLMAX;

      if (m_state == YSFRXS_NONE)
        processNone(samples[i]);
      else
        processData(samples[i]);
    }
  }
}

void CYSFRX::processNone(q15_t sample)
{
  m_symbolBuffer <<= 1;
  if (sample < 0)
    m_symbolBuffer |= 0x01U;

  m_symbols[m_symbolPtr] = sample;

  // Fuzzy matching of the data sync bit sequence
  if (countBits32((m_symbolBuffer & YSF_SYNC_SYMBOLS_MASK) ^ YSF_SYNC_SYMBOLS) <= SYNC_SYMBOL_ERRS) {
    uint16_t ptr = m_symbolPtr + 1U;
    if (ptr >= YSF_SYNC_LENGTH_SYMBOLS)
      ptr = 0U;

    q15_t max  = -16000;
    q15_t min  =  16000;

    for (uint8_t i = 0U; i < YSF_SYNC_LENGTH_SYMBOLS; i++) {
      if (m_symbols[ptr] > max)
        max = m_symbols[ptr];
      if (m_symbols[ptr] < min)
        min = m_symbols[ptr];

      ptr++;
      if (ptr >= YSF_SYNC_LENGTH_SYMBOLS)
        ptr = 0U;
    }

    q15_t centre = (max + min) >> 1;

    q31_t v1 = (max - centre) * SCALING_FACTOR;
    q15_t threshold = q15_t(v1 >> 15);

    ptr = m_symbolPtr + 1U;
    if (ptr >= YSF_SYNC_LENGTH_SYMBOLS)
      ptr = 0U;

    for (uint8_t i = 0U; i < YSF_SYNC_LENGTH_SYMBOLS; i++) {
      q15_t sample = m_symbols[ptr] - centre;

      if (sample < -threshold) {
        m_bitBuffer <<= 2;
        m_bitBuffer |= 0x01U;
      } else if (sample < 0) {
        m_bitBuffer <<= 2;
        m_bitBuffer |= 0x00U;
      } else if (sample < threshold) {
        m_bitBuffer <<= 2;
        m_bitBuffer |= 0x02U;
      } else {
        m_bitBuffer <<= 2;
        m_bitBuffer |= 0x03U;
      }

      ptr++;
      if (ptr >= YSF_SYNC_LENGTH_SYMBOLS)
        ptr = 0U;
    }

    // Fuzzy matching of the data sync bit sequence
    if (countBits64((m_bitBuffer & YSF_SYNC_BITS_MASK) ^ YSF_SYNC_BITS) <= SYNC_BIT_ERRS) {
      DEBUG5("YSFRX: sync found in None min/max/centre/threshold", min, max, centre, threshold);
      for (uint8_t i = 0U; i < YSF_SYNC_LENGTH_BYTES; i++)
        m_buffer[i] = YSF_SYNC_BYTES[i];
      m_centre    = centre;
      m_threshold = threshold;
      m_lostCount = MAX_SYNC_FRAMES;
      m_bufferPtr = YSF_SYNC_LENGTH_BITS;
      m_state     = YSFRXS_DATA;
      io.setDecode(true);
    }
  }

  m_symbolPtr++;
  if (m_symbolPtr >= YSF_SYNC_LENGTH_SYMBOLS)
    m_symbolPtr = 0U;
}

void CYSFRX::processData(q15_t sample)
{
  sample -= m_centre;

  if (sample < -m_threshold) {
    m_bitBuffer <<= 2;
    m_bitBuffer |= 0x01U;
    WRITE_BIT1(m_buffer, m_bufferPtr, false);
    m_bufferPtr++;
    WRITE_BIT1(m_buffer, m_bufferPtr, true);
    m_bufferPtr++;
  } else if (sample < 0) {
    m_bitBuffer <<= 2;
    m_bitBuffer |= 0x00U;
    WRITE_BIT1(m_buffer, m_bufferPtr, false);
    m_bufferPtr++;
    WRITE_BIT1(m_buffer, m_bufferPtr, false);
    m_bufferPtr++;
  } else if (sample < m_threshold) {
    m_bitBuffer <<= 2;
    m_bitBuffer |= 0x02U;
    WRITE_BIT1(m_buffer, m_bufferPtr, true);
    m_bufferPtr++;
    WRITE_BIT1(m_buffer, m_bufferPtr, false);
    m_bufferPtr++;
  } else {
    m_bitBuffer <<= 2;
    m_bitBuffer |= 0x03U;
    WRITE_BIT1(m_buffer, m_bufferPtr, true);
    m_bufferPtr++;
    WRITE_BIT1(m_buffer, m_bufferPtr, true);
    m_bufferPtr++;
  }

  // Only search for a sync in the right place +-1 symbol
  if (m_bufferPtr >= (YSF_SYNC_LENGTH_BITS - 2U) && m_bufferPtr <= (YSF_SYNC_LENGTH_BITS + 2U)) {
    // Fuzzy matching of the data sync bit sequence
    if (countBits64((m_bitBuffer & YSF_SYNC_BITS_MASK) ^ YSF_SYNC_BITS) <= SYNC_BIT_ERRS) {
#if defined(WANT_DEBUG)
      if (m_bufferPtr < YSF_SYNC_LENGTH_BITS)
        DEBUG2("YSFRX: found sync in Data, early", YSF_SYNC_LENGTH_BITS - m_bufferPtr);
      else if (m_bufferPtr > YSF_SYNC_LENGTH_BITS)
        DEBUG2("YSFRX: found sync in Data, late", m_bufferPtr - YSF_SYNC_LENGTH_BITS);
      else
        DEBUG1("YSFRX: found sync in Data");
#endif
      m_lostCount = MAX_SYNC_FRAMES;
      m_bufferPtr = YSF_SYNC_LENGTH_BITS;
    }
  }

  // Send a data frame to the host if the required number of bits have been received, or if a data sync has been seen
  if (m_bufferPtr == YSF_FRAME_LENGTH_BITS) {
    // We've not seen a data sync for too long, signal RXLOST and change to RX_NONE
    m_lostCount--;
    if (m_lostCount == 0U) {
      DEBUG1("YSFRX: sync timed out, lost lock");
      io.setDecode(false);

      serial.writeYSFLost();

      m_state = YSFRXS_NONE;
      return;
    } else {
      uint8_t FICH[6U];
      bool ok = rxFICH(m_buffer + YSF_SYNC_LENGTH_BYTES, FICH);

#if defined(WANT_DEBUG)
      if (ok)
        DEBUG5("YSFRX: Valid FICH", FICH[0U], FICH[1U], FICH[2U], FICH[3U]);
      else
        DEBUG5("YSFRX: Invalid FICH", FICH[0U], FICH[1U], FICH[2U], FICH[3U]);
#endif

      m_outBuffer[0U]  = (FICH[0U] & 0xC0U) << 0;		// FI
      m_outBuffer[0U] |= (FICH[2U] & 0x03U) << 4;		// DT
      m_outBuffer[0U] |= ok ? 0x01U : 0x00U;

      serial.writeYSFData(m_outBuffer, YSF_FRAME_LENGTH_BYTES + 1U);

      if (ok && (FICH[0U] & 0xC0U) == 0x80U) {
        DEBUG1("YSFRX: end of transmission");
        io.setDecode(false);
        m_state = YSFRXS_NONE;
      } else {
        // Start the next frame
        ::memset(m_outBuffer, 0x00U, YSF_FRAME_LENGTH_BYTES + 1U);
        m_bufferPtr = 0U;
      }
    }
  }
}

const unsigned int NUM_OF_STATES_D2 = 8U;
const unsigned int NUM_OF_STATES = 16U;
const uint32_t M = 3U;
const unsigned int K = 5U;

bool CYSFRX::rxFICH(uint8_t* data, uint8_t* FICH)
{
  ::memset(m_metrics1, 0x00U, NUM_OF_STATES * sizeof(uint32_t));
  m_oldMetrics = m_metrics1;
  m_newMetrics = m_metrics2;
  m_dp         = m_decisions;

  uint8_t m = 0U;
  // Deinterleave the FICH and send bits to the Viterbi decoder
  for (uint8_t i = 0U; i < 100U; i++) {
    uint8_t n = INTERLEAVE_TABLE_RX[i];
    uint8_t s0 = READ_BIT1(data, n) ? 1U : 0U;

    n++;
    uint8_t s1 = READ_BIT1(data, n) ? 1U : 0U;

    viterbiDecode(s0, s1);
  }

  uint8_t output[13U];
  chainback(output);

  uint32_t b0 = golay24128(output + 0U);
  uint32_t b1 = golay24128(output + 3U);
  uint32_t b2 = golay24128(output + 6U);
  uint32_t b3 = golay24128(output + 9U);

  FICH[0U] = (b0 >> 16) & 0xFFU;
  FICH[1U] = ((b0 >> 8) & 0xF0U) | ((b1 >> 20) & 0x0FU);
  FICH[2U] = (b1 >> 12) & 0xFFU;
  FICH[3U] = (b2 >> 16) & 0xFFU;
  FICH[4U] = ((b2 >> 8) & 0xF0U) | ((b3 >> 20) & 0x0FU);
  FICH[5U] = (b3 >> 12) & 0xFFU;

  return checksum(FICH);
}

void CYSFRX::viterbiDecode(uint8_t s0, uint8_t s1)
{
  *m_dp = 0U;

  for (uint8_t i = 0U; i < NUM_OF_STATES_D2; i++) {
    uint8_t j = i * 2U;

    uint16_t metric = (BRANCH_TABLE1[i] ^ s0) + (BRANCH_TABLE2[i] ^ s1);

    uint16_t m0 = m_oldMetrics[i] + metric;
    uint16_t m1 = m_oldMetrics[i + NUM_OF_STATES_D2] + (M - metric);
    uint8_t decision0 = (m0 >= m1) ? 1U : 0U;
    m_newMetrics[j + 0U] = decision0 != 0U ? m1 : m0;

    m0 = m_oldMetrics[i] + (M - metric);
    m1 = m_oldMetrics[i + NUM_OF_STATES_D2] + metric;
    uint8_t decision1 = (m0 >= m1) ? 1U : 0U;
    m_newMetrics[j + 1U] = decision1 != 0U ? m1 : m0;

    *m_dp |= (uint64_t(decision1) << (j + 1U)) | (uint64_t(decision0) << (j + 0U));
  }

  ++m_dp;

  uint16_t* tmp = m_oldMetrics;
  m_oldMetrics = m_newMetrics;
  m_newMetrics = tmp;
}

void CYSFRX::chainback(uint8_t *out)
{
  uint32_t state = 0U;

  uint8_t nbits = 96U;
  while (nbits-- > 0) {
    --m_dp;

    uint32_t i = state >> (9 - K);
    uint8_t bit = uint8_t(*m_dp >> i) & 1;
    state = (bit << 7) | (state >> 1);

    WRITE_BIT1(out, nbits, bit != 0U);
  }
}

#define X22             0x00400000   /* vector representation of X^{22} */
#define X11             0x00000800   /* vector representation of X^{11} */
#define MASK12          0xfffff800   /* auxiliary vector for testing */
#define GENPOL          0x00000c75   /* generator polinomial, g(x) */

uint32_t CYSFRX::getSyndrome23127(uint32_t pattern) const
/*
 * Compute the syndrome corresponding to the given pattern, i.e., the
 * remainder after dividing the pattern (when considering it as the vector
 * representation of a polynomial) by the generator polynomial, GENPOL.
 * In the program this pattern has several meanings: (1) pattern = infomation
 * bits, when constructing the encoding table; (2) pattern = error pattern,
 * when constructing the decoding table; and (3) pattern = received vector, to
 * obtain its syndrome in decoding.
 */
{
  uint32_t aux = X22;
 
  if (pattern >= X11) {
    while (pattern & MASK12) {
      while (!(aux & pattern))
        aux = aux >> 1;

        pattern ^= (aux / X11) * GENPOL;
    }
  }

  return pattern;
}

uint32_t CYSFRX::golay24128(const uint8_t* bytes) const
{
  uint32_t code = bytes[0U];
  code <<= 8;
  code |= bytes[1U];
  code <<= 8;
  code |= bytes[2U];

  code >>= 1;

  uint32_t syndrome = getSyndrome23127(code);
  uint32_t error_pattern = DECODING_TABLE_23127[syndrome];

  code ^= error_pattern;
  code <<= 1;

  return code;
}

bool CYSFRX::checksum(const uint8_t* fich) const
{
  union {
    uint16_t crc16;
    uint8_t  crc8[2U];
  };

  crc16 = 0U;
  crc16 = (uint16_t(crc8[0U]) << 8) ^ CCITT_TABLE[crc8[1U] ^ fich[0U]];
  crc16 = (uint16_t(crc8[0U]) << 8) ^ CCITT_TABLE[crc8[1U] ^ fich[1U]];
  crc16 = (uint16_t(crc8[0U]) << 8) ^ CCITT_TABLE[crc8[1U] ^ fich[2U]];
  crc16 = (uint16_t(crc8[0U]) << 8) ^ CCITT_TABLE[crc8[1U] ^ fich[3U]];

  crc16 = ~crc16;

  return crc8[0U] == fich[5U] && crc8[1U] == fich[4U];
}

