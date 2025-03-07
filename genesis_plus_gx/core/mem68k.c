/***************************************************************************************
 *  Genesis Plus
 *  Main 68k bus handlers
 *
 *  Copyright (C) 1998-2003  Charles Mac Donald (original code)
 *  Copyright (C) 2007-2024  Eke-Eke (Genesis Plus GX)
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************************/

#include "shared.h"

/*--------------------------------------------------------------------------*/
/* Unused areas (return open bus data, i.e prefetched instruction word)     */
/*--------------------------------------------------------------------------*/

unsigned int m68k_read_bus_8(unsigned int address)
{
#ifdef LOGERROR
  error("Unused read8 %08X (%08X)\n", address, m68k_get_reg(M68K_REG_PC));
#endif
  address = m68k.pc | (address & 1);
  return READ_BYTE(m68k.memory_map[((address)>>16)&0xff].base, (address) & 0xffff);
}

unsigned int m68k_read_bus_16(unsigned int address)
{
#ifdef LOGERROR
  error("Unused read16 %08X (%08X)\n", address, m68k_get_reg(M68K_REG_PC));
#endif
  address = m68k.pc;
  return *(uint16 *)(m68k.memory_map[((address)>>16)&0xff].base + ((address) & 0xffff));
}


void m68k_unused_8_w(unsigned int address, unsigned int data)
{
#ifdef LOGERROR
  error("Unused write8 %08X = %02X (%08X)\n", address, data, m68k_get_reg(M68K_REG_PC));
#endif
}

void m68k_unused_16_w(unsigned int address, unsigned int data)
{
#ifdef LOGERROR
  error("Unused write16 %08X = %04X (%08X)\n", address, data, m68k_get_reg(M68K_REG_PC));
#endif
}


/*--------------------------------------------------------------------------*/
/* Illegal areas (cause system to lock-up since !DTACK is not returned)     */
/*--------------------------------------------------------------------------*/

void m68k_lockup_w_8 (unsigned int address, unsigned int data)
{
#ifdef LOGERROR
  error ("Lockup %08X = %02X (%08X)\n", address, data, m68k_get_reg(M68K_REG_PC));
#endif
  if (!config.force_dtack)
  {
    m68k_pulse_halt();
    m68k.cycles = m68k.cycle_end;
  }
}

void m68k_lockup_w_16 (unsigned int address, unsigned int data)
{
#ifdef LOGERROR
  error ("Lockup %08X = %04X (%08X)\n", address, data, m68k_get_reg(M68K_REG_PC));
#endif
  if (!config.force_dtack)
  {
    m68k_pulse_halt();
    m68k.cycles = m68k.cycle_end;
  }
}

unsigned int m68k_lockup_r_8 (unsigned int address)
{ 
#ifdef LOGERROR
  error ("Lockup %08X.b (%08X)\n", address, m68k_get_reg(M68K_REG_PC));
#endif
  if (!config.force_dtack)
  {
    m68k_pulse_halt();
    m68k.cycles = m68k.cycle_end;
  }
  address = m68k.pc | (address & 1);
  return READ_BYTE(m68k.memory_map[((address)>>16)&0xff].base, (address) & 0xffff);
}

unsigned int m68k_lockup_r_16 (unsigned int address)
{
#ifdef LOGERROR
  error ("Lockup %08X.w (%08X)\n", address, m68k_get_reg(M68K_REG_PC));
#endif
  if (!config.force_dtack)
  {
    m68k_pulse_halt();
    m68k.cycles = m68k.cycle_end;
  }
  address = m68k.pc;
  return *(uint16 *)(m68k.memory_map[((address)>>16)&0xff].base + ((address) & 0xffff));
}


/*--------------------------------------------------------------------------*/
/* Z80 bus (accessed through I/O chip)                                      */
/*--------------------------------------------------------------------------*/

unsigned int z80_read_byte(unsigned int address)
{
  /* Z80 bus access latency */
  m68k.cycles += 1 * 7;

  switch ((address >> 13) & 3)
  {
    case 2:   /* YM2612 */
    {
      return fm_read(m68k.cycles, address & 3);
    }

    case 3:   /* Misc  */
    {
      /* VDP (through 68k bus) */
      if ((address & 0xFF00) == 0x7F00)
      {
        return m68k_lockup_r_8(address);
      }
      return (m68k_read_bus_8(address) | 0xFF);
    }

    default: /* ZRAM */
    {
      return zram[address & 0x1FFF];
    }
  }
}

unsigned int z80_read_word(unsigned int address)
{
  unsigned int data = z80_read_byte(address);
  return (data | (data << 8));
}

void z80_write_byte(unsigned int address, unsigned int data)
{
  /* Z80 bus access latency (fixes Pacman 2: New Adventures sound engine crashes & Puyo Puyo 2 crash when exiting option menu) */
  m68k.cycles += 1 * 7;

  switch ((address >> 13) & 3)
  {
    case 2: /* YM2612 */
    {
      fm_write(m68k.cycles, address & 3, data);
      return;
    }

    case 3:
    {
      switch ((address >> 8) & 0x7F)
      {
        case 0x60:  /* Bank register */
        {
          gen_zbank_w(data & 1);
          return;
        }

        case 0x7F:  /* VDP */
        {
          m68k_lockup_w_8(address, data);
          return;
        }

        default:
        {
          m68k_unused_8_w(address, data);
          return;
        }
      }
    }
      
    default: /* ZRAM */
    {
      zram[address & 0x1FFF] = data;
      return;
    }
  }
}

void z80_write_word(unsigned int address, unsigned int data)
{
  z80_write_byte(address, data >> 8);
}


/*--------------------------------------------------------------------------*/
/* MAIN-CPU polling detection and SUB-CPU synchronization (MEGA CD mode)    */
/*--------------------------------------------------------------------------*/

static void m68k_poll_detect(unsigned int reg_mask)
{
  /* detect MAIN-CPU register polling */
  if (m68k.poll.detected & reg_mask)
  {
    if (m68k.cycles <= m68k.poll.cycle)
    {
      if (m68k.pc == m68k.poll.pc)
      {
        /* MAIN-CPU polling confirmed ? */
        if (m68k.poll.detected & 1)
        {
          /* idle MAIN-CPU until register is modified */
#ifdef LOG_SCD
          error("m68k stopped from %d cycles\n", m68k.cycles);
#endif
          m68k.cycles = m68k.cycle_end;
          m68k.stopped = reg_mask;
        }
        else
        {
          /* confirm MAIN-CPU polling */
          m68k.poll.detected |= 1;
          m68k.poll.cycle = m68k.cycles + 840;
        }
      }
      return;
    }
  }
  else
  {
    /* set MAIN-CPU register access flag */
    m68k.poll.detected = reg_mask;
  }

  /* reset MAIN-CPU polling detection */
  m68k.poll.cycle = m68k.cycles + 840;
  m68k.poll.pc = m68k.pc;
}

static void m68k_poll_sync(unsigned int reg_mask)
{
  /* relative SUB-CPU cycle counter */
  unsigned int cycles = (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE;

  if (!s68k.stopped)
  {
    /* save current SUB-CPU end cycle count (recursive execution is possible) */
    int end_cycle = s68k.cycle_end;

    /* sync SUB-CPU with MAIN-CPU */
    s68k_run(cycles);

    /* restore SUB-CPU end cycle count */
    s68k.cycle_end = end_cycle;
  }

  /* SUB-CPU idle on register polling ? */
  if (s68k.stopped & reg_mask)
  {
    /* sync SUB-CPU with MAIN-CPU */
    s68k.cycles = cycles;

    /* restart SUB-CPU */
    s68k.stopped = 0;
#ifdef LOG_SCD
    error("s68k started from %d cycles\n", cycles);
#endif
  }

  /* clear CPU register access flags */
  s68k.poll.detected &= ~reg_mask;
  m68k.poll.detected &= ~reg_mask;
}

static void s68k_sync(void)
{
  if (!s68k.stopped)
  {
    /* relative SUB-CPU cycle counter */
    unsigned int cycles = (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE;

    /* save current SUB-CPU end cycle count (recursive execution is possible) */
    int end_cycle = s68k.cycle_end;

    /* sync SUB-CPU with MAIN-CPU */
    s68k_run(cycles);

    /* restore SUB-CPU end cycle count */
    s68k.cycle_end = end_cycle;
  }
}

/*--------------------------------------------------------------------------*/
/* I/O Control                                                              */
/*--------------------------------------------------------------------------*/

unsigned int ctrl_io_read_byte(unsigned int address)
{
  switch ((address >> 8) & 0xFF)
  {
    case 0x00:  /* I/O chip */
    {
      if (!(address & 0xE0))
      {
        return io_68k_read((address >> 1) & 0x0F);
      }
      return m68k_read_bus_8(address);
    }

    case 0x11:  /* Z80 BUSACK */
    {
      if (!(address & 1))
      {
        /* Unused bits return prefetched bus data (Time Killers) */
        address = m68k.pc;

        /* Check if bus has been requested and is not reseted */
        if (zstate == 3)
        {
          /* D0 is cleared */
          return (READ_BYTE(m68k.memory_map[((address)>>16)&0xff].base, (address) & 0xffff) & 0xFE);
        }

        /* D0 is set */
        return (READ_BYTE(m68k.memory_map[((address)>>16)&0xff].base, (address) & 0xffff) | 0x01);
      }
      return m68k_read_bus_8(address);
    }

    case 0x20:  /* MEGA-CD */
    {
#ifdef LOG_SCD
      error("[%d][%d]read byte CD register %X (%X)\n", v_counter, m68k.cycles, address, m68k.pc);
#endif
      if (system_hw == SYSTEM_MCD)
      {
        /* register index ($A12000-A1203F mirrored up to $A120FF) */
        uint8 index = address & 0x3f;

        /* Memory Mode */
        if (index == 0x03)
        {
          m68k_poll_detect(1<<0x03);

          /* mask PM0 and PM1 bits on MAIN-CPU side */
          return scd.regs[0x03>>1].byte.l & 0xc7;
        }

        /* CDC Mode */
        if (index == 0x04)
        {
          /* sync SUB-CPU with MAIN-CPU (fixes MCD-verificator CDC DMA3 Test #2) */
          s68k_sync();
          return scd.regs[0x04>>1].byte.h & 0xc7;
        }

        /* CDC register address (not accessible from MAIN-CPU) */
        if (index == 0x05)
        {
          return 0x00;
        }

        /* SUB-CPU communication flags */
        if (index == 0x0f)
        {
          /* sync SUB-CPU with MAIN-CPU (fixes Dracula Unleashed with Sega CD Model 2 Boot ROM) */
          s68k_sync();
          m68k_poll_detect(1<<0x0f);
          return scd.regs[0x0f>>1].byte.l;
        }

        /* default registers */
        if (index < 0x30)
        {
          /* SUB-CPU communication words */
          if (index >= 0x20)
          {
            m68k_poll_detect(1 << (index - 0x10));
          }

          /* register LSB */
          if (address & 1)
          {
            return scd.regs[index >> 1].byte.l;
          }

          /* register MSB */
          return scd.regs[index >> 1].byte.h;
        }
      }

      return m68k_read_bus_8(address); 
    }

    case 0x30:  /* TIME */
    {
      if (cart.hw.time_r)
      {
        unsigned int data = cart.hw.time_r(address);
        if (address & 1)
        {
          return (data & 0xFF);
        }
        return (data >> 8);
      }
      return m68k_read_bus_8(address);
    }

    case 0x41:  /* BOOT ROM */
    {
      if ((config.bios & 1) && (address & 1))
      {
        unsigned int data = gen_bankswitch_r() & 1;

        /* Unused bits return prefetched bus data */
        address = m68k.pc;
        data |= (READ_BYTE(m68k.memory_map[((address)>>16)&0xff].base, (address) & 0xffff) & 0xFE);
        return data;
      }
      return m68k_read_bus_8(address);
    }

    case 0x50:  /* SVP */
    {
      if ((address & 0xFC) == 0x00)
      {
        unsigned int data = svp->ssp1601.gr[SSP_XST].byte.h;
        return (address & 1) ? (data & 0xFF) : (data >> 8);
      }

      if ((address & 0xFE) == 0x04)
      {
        unsigned int data = svp->ssp1601.gr[SSP_PM0].byte.h;
        svp->ssp1601.gr[SSP_PM0].byte.h &= ~1;
        return (address & 1) ? (data & 0xFF) : (data >> 8);
      }

      return m68k_read_bus_8(address);
    }

    case 0x10:  /* MEMORY MODE */
    case 0x12:  /* Z80 RESET */
    case 0x13:  /* unknown */
    case 0x40:  /* TMSS */
    case 0x44:  /* RADICA */
    {
      return m68k_read_bus_8(address);
    }

    default:  /* Invalid address */
    {
      return m68k_lockup_r_8(address);
    }
  }
}

unsigned int ctrl_io_read_word(unsigned int address)
{
  switch ((address >> 8) & 0xFF)
  {
    case 0x00:  /* I/O chip */
    {
      if (!(address & 0xE0))
      {
        unsigned int data = io_68k_read((address >> 1) & 0x0F);
        return (data << 8 | data);
      }
      return m68k_read_bus_16(address); 
    }

    case 0x11:  /* Z80 BUSACK */
    {
      /* Unused bits return prefetched bus data (Time Killers) */
      address = m68k.pc;

      /* Check if bus has been requested and is not reseted */
      if (zstate == 3)
      {
        /* D8 is cleared */
        return (*(uint16 *)(m68k.memory_map[((address)>>16)&0xff].base + ((address) & 0xffff)) & 0xFEFF);
      }

      /* D8 is set */
      return (*(uint16 *)(m68k.memory_map[((address)>>16)&0xff].base + ((address) & 0xffff)) | 0x0100);
    }

    case 0x20:  /* MEGA-CD */
    {
#ifdef LOG_SCD
      error("[%d][%d]read word CD register %X (%X)\n", v_counter, m68k.cycles, address, m68k.pc);
#endif
      if (system_hw == SYSTEM_MCD)
      {
        /* register index ($A12000-A1203F mirrored up to $A120FF) */
        uint8 index = address & 0x3f;

        /* Memory Mode */
        if (index == 0x02)
        {
          m68k_poll_detect(1<<0x03);

          /* mask PM0 and PM1 bits on MAIN-CPU side */
          return scd.regs[0x03>>1].w & 0xffc7;
        }

        /* CDC host data (word access only ?) */
        if (index == 0x08)
        {
          /* sync SUB-CPU with MAIN-CPU if CDC data transfer is not yet enabled (fixes MCD-verificator CDC INIT Test #4) */
          if (!(scd.regs[0x04>>1].byte.h & 0x40))
          {
            s68k_sync();
          }
          return cdc_host_r(CDC_MAIN_CPU_ACCESS);
        }

        /* H-INT vector (word access only ?) */
        if (index == 0x06)
        {
          return *(uint16 *)(m68k.memory_map[scd.cartridge.boot].base + 0x72);
        }

        /* Stopwatch counter (word read access only ?) */
        if (index == 0x0c)
        {
          /* relative SUB-CPU cycle counter */
          unsigned int cycles = (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE;

          /* cycle-accurate counter value */
          return (scd.regs[0x0c>>1].w + ((cycles - scd.stopwatch) / TIMERS_SCYCLES_RATIO)) & 0xfff;
        }

        /* CDC Mode (CDC register address not accessible from MAIN-CPU) */
        if (index == 0x04)
        {
          return (scd.regs[index >> 1].byte.h << 8);
        }

        /* default registers */
        if (index < 0x30)
        {
          /* SUB-CPU communication words */
          if (index >= 0x20)
          {
            /* sync SUB-CPU with MAIN-CPU (fixes Soul Star) */
            s68k_sync();
            m68k_poll_detect(3 << (index - 0x10));
          }
          
          return scd.regs[index >> 1].w;
        }
      }

      /* invalid address */
      return m68k_read_bus_16(address); 
    }

    case 0x30:  /* TIME */
    {
      if (cart.hw.time_r)
      {
        return cart.hw.time_r(address);
      }
      return m68k_read_bus_16(address); 
    }

    case 0x50:  /* SVP */
    {
      if ((address & 0xFC) == 0x00)
      {
        return svp->ssp1601.gr[SSP_XST].byte.h;
      }

      if ((address & 0xFE) == 0x04)
      {
        unsigned int data = svp->ssp1601.gr[SSP_PM0].byte.h;
        svp->ssp1601.gr[SSP_PM0].byte.h &= ~1;
        return data;
      }

      return m68k_read_bus_16(address);
    }

    case 0x10:  /* MEMORY MODE */
    case 0x12:  /* Z80 RESET */
    case 0x13:  /* unknown */
    case 0x40:  /* TMSS */
    case 0x41:  /* BOOT ROM */
    case 0x44:  /* RADICA */
    {
      return m68k_read_bus_16(address);
    }

    default:  /* Invalid address */
    {
      return m68k_lockup_r_16(address);
    }
  }
}

void ctrl_io_write_byte(unsigned int address, unsigned int data)
{
  switch ((address >> 8) & 0xFF)
  {
    case 0x00:  /* I/O chip */
    {
      if ((address & 0xE1) == 0x01)
      {
        /* get /LWR only */
        io_68k_write((address >> 1) & 0x0F, data);
        return;
      }
      m68k_unused_8_w(address, data);
      return;
    }

    case 0x11:  /* Z80 BUSREQ */
    {
      if (!(address & 1))
      {
        gen_zbusreq_w(data & 1, m68k.cycles);
        return;
      }
      m68k_unused_8_w(address, data);
      return;
    }

    case 0x12:  /* Z80 RESET */
    {
      if (!(address & 1))
      {
        gen_zreset_w(data & 1, m68k.cycles);
        return;
      }
      m68k_unused_8_w(address, data);
      return;
    }

    case 0x20:  /* MEGA-CD */
    {
#ifdef LOG_SCD
      error("[%d][%d]write byte CD register %X -> 0x%02X (%X)\n", v_counter, m68k.cycles, address, data, m68k.pc);
#endif
      if (system_hw == SYSTEM_MCD)
      {
        /* register index ($A12000-A1203F mirrored up to $A120FF) */
        switch (address & 0x3f)
        {
          case 0x00:  /* SUB-CPU interrupt */
          {
            /* IFL2 bit */
            if (data & 0x01)
            {
              /* level 2 interrupt enabled ? */
              if (scd.regs[0x32>>1].byte.l & 0x04)
              {
                /* sync SUB-CPU with MAIN-CPU (fixes Earnest Evans, Fhey Area) */
                s68k_sync();

                /* set IFL2 flag */
                scd.regs[0x00].byte.h |= 0x01;

                /* trigger level 2 interrupt */
                scd.pending |= (1 << 2);

                /* update IRQ level */
                s68k_update_irq((scd.pending & scd.regs[0x32>>1].byte.l) >> 1);
              }
            }

            /* writing 0 does nothing */
            return;
          }

          case 0x01:  /* SUB-CPU control */
          {
            unsigned int halted = s68k.stopped;

            /* RESET bit */
            if (data & 0x01)
            {
              /* SUB-CPU reset is triggered on /RESET input 0->1 transition */
              if (!(scd.regs[0x00].byte.l & 0x01))
              {
                s68k_pulse_reset();
              }

              /* BUSREQ bit */
              if (data & 0x02)
              {
                /* SUB-CPU is halted (/HALT input is asserted) */
                s68k_pulse_halt();
              }
              else
              {
                /* SUB-CPU is running (/HALT input is released) */
                s68k_clear_halt();
              }

              /* update BUSREQ and RESET bits */
              scd.regs[0x00].byte.l = data & 0x03;
            }
            else
            {
              /* SUB-CPU is halted (/HALT and /RESET inputs are asserted) */
              s68k_pulse_halt();

              /* RESET bit is cleared and BUSREQ bit is set to 1 (verified on real hardware) */
              scd.regs[0x00].byte.l = 0x02;
            }

            /* BUSREQ bit remains set to 0 if SUB-CPU is halted while stopped (verified on real hardware) */
            if (s68k.stopped & 0x01)
            {
              scd.regs[0x00].byte.l &= ~0x02;
            }

            /* check if SUB-CPU halt status has changed */
            if (s68k.stopped != halted)
            {
              /* PRG-RAM (128KB bank) is normally mapped to $020000-$03FFFF (resp. $420000-$43FFFF) */
              unsigned int base = scd.cartridge.boot + 0x02;

              /* PRG-RAM can only be accessed from MAIN-CPU & Z80 when BUSREQ bit is set (Dungeon Explorer USA version) */
              if (scd.regs[0x00].byte.l & 0x02)
              {
                m68k.memory_map[base].read8   = m68k.memory_map[base+1].read8   = NULL;
                m68k.memory_map[base].read16  = m68k.memory_map[base+1].read16  = NULL;
                m68k.memory_map[base].write8  = m68k.memory_map[base+1].write8  = NULL;
                m68k.memory_map[base].write16 = m68k.memory_map[base+1].write16 = NULL;
                zbank_memory_map[base].read   = zbank_memory_map[base+1].read   = NULL;
                zbank_memory_map[base].write  = zbank_memory_map[base+1].write  = NULL;

                /* check if CDC DMA to PRG-RAM is running */
                if (cdc.dma_w == prg_ram_dma_w)
                {
                  /* synchronize CDC DMA with MAIN-CPU */
                  cdc_dma_update((m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE);

                  /* halt CDC DMA to PRG-RAM (if still running) */
                  cdc.halted_dma_w = cdc.dma_w;
                  cdc.dma_w = 0;
                }
              }
              else
              {
                m68k.memory_map[base].read8   = m68k.memory_map[base+1].read8   = m68k_read_bus_8;
                m68k.memory_map[base].read16  = m68k.memory_map[base+1].read16  = m68k_read_bus_16;
                m68k.memory_map[base].write8  = m68k.memory_map[base+1].write8  = m68k_unused_8_w;
                m68k.memory_map[base].write16 = m68k.memory_map[base+1].write16 = m68k_unused_16_w;
                zbank_memory_map[base].read   = zbank_memory_map[base+1].read   = zbank_unused_r;
                zbank_memory_map[base].write  = zbank_memory_map[base+1].write  = zbank_unused_w;

                /* check if CDC DMA to PRG-RAM is halted */
                if (cdc.halted_dma_w == prg_ram_dma_w)
                {
                  /* relative SUB-CPU cycle counter */
                  unsigned int cycles = (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE;

                  /* enable CDC DMA to PRG-RAM */
                  cdc.dma_w = prg_ram_dma_w;
                  cdc.halted_dma_w = 0;

                  /* synchronize CDC DMA with MAIN-CPU (only if not already ahead) */
                  if (cdc.cycles[0] < cycles)
                  {
                    cdc.cycles[0] = cycles;
                  }
                }
              }
            }

            return;
          }

          case 0x02:  /* PRG-RAM Write Protection */
          {
            scd.regs[0x02>>1].byte.h = data;
            return;
          }

          case 0x03:  /* Memory mode */
          {
            m68k_poll_sync(1<<0x03);

            /* PRG-RAM 128k bank mapped to $020000-$03FFFF (resp. $420000-$43FFFF) */
            m68k.memory_map[scd.cartridge.boot + 0x02].base = scd.prg_ram + ((data & 0xc0) << 11);
            m68k.memory_map[scd.cartridge.boot + 0x03].base = m68k.memory_map[scd.cartridge.boot + 0x02].base + 0x10000;

            /* check current mode */
            if (scd.regs[0x03>>1].byte.l & 0x04)
            {
              /* DMNA bit */
              if (data & 0x02)
              {
                /* writing 1 to DMNA in 1M mode will return Word-RAM to SUB-CPU in 2M mode */
                scd.dmna = 1;
              }
              else
              {
                /* writing 0 to DMNA in 1M mode actually sets DMNA bit */
                data |= 0x02;

                /* update BK0-1 & DMNA bits */
                scd.regs[0x03>>1].byte.l = (scd.regs[0x03>>1].byte.l & ~0xc2) | (data & 0xc2);
                return;
              }
            }
            else
            {
              /* writing 0 to DMNA in 2M mode does nothing */
              if (data & 0x02)
              {
                int i;

                /* Word-RAM is assigned to SUB-CPU */
                scd.dmna = 1;

                /* MAIN-CPU: $200000-$23FFFF is unmapped */
                for (i=scd.cartridge.boot+0x20; i<scd.cartridge.boot+0x24; i++)
                {
                  m68k.memory_map[i].read8   = m68k_read_bus_8;
                  m68k.memory_map[i].read16  = m68k_read_bus_16;
                  m68k.memory_map[i].write8  = m68k_unused_8_w;
                  m68k.memory_map[i].write16 = m68k_unused_16_w;
                  zbank_memory_map[i].read   = zbank_unused_r;
                  zbank_memory_map[i].write  = zbank_unused_w;
                }

                /* SUB-CPU: access to Word-RAM at 0x080000-0x0BFFFF is unlocked (/DTACK asserted) */
                for (i=0x08; i<0x0c; i++)
                {
                  s68k.memory_map[i].read8   = NULL;
                  s68k.memory_map[i].read16  = NULL;
                  s68k.memory_map[i].write8  = NULL;
                  s68k.memory_map[i].write16 = NULL;
                }

                /* clear RET bit and update BK0-1 & DMNA bits */
                scd.regs[0x03>>1].byte.l = (scd.regs[0x03>>1].byte.l & ~0xc3) | (data & 0xc2);

                /* check if SUB-CPU is waiting for Word-RAM access */
                if (s68k.stopped & 0x04)
                {
                  /* synchronize SUB-CPU with MAIN-CPU */
                  s68k.cycles = (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE;

                  /* restart SUB-CPU */
                  s68k_clear_wait();
#ifdef LOG_SCD
                  error("s68k started from %d cycles\n", s68k.cycles);
#endif
                }

                /* check if graphics operation is started */
                if (scd.regs[0x58>>1].byte.h & 0x80)
                {
                  /* relative SUB-CPU cycle counter */
                  unsigned int cycles = (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE;

                  /* synchronize GFX processing with MAIN-CPU (only if not already ahead) */
                  if (gfx.cycles < cycles)
                  {
                    gfx.cycles = cycles;
                  }
                }

                /* check if CDC DMA to 2M Word-RAM is halted */
                if (cdc.halted_dma_w == word_ram_2M_dma_w)
                {
                  /* relative SUB-CPU cycle counter */
                  unsigned int cycles = (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE;

                  /* enable CDC DMA to 2M Word-RAM */
                  cdc.dma_w = word_ram_2M_dma_w;
                  cdc.halted_dma_w = 0;

                  /* synchronize CDC DMA with MAIN-CPU (only if not already ahead) */
                  if (cdc.cycles[0] < cycles)
                  {
                    cdc.cycles[0] = cycles;
                  }
                }

                return;
              }
            }

            /* update BK0-1 bits only */
            scd.regs[0x03>>1].byte.l = (scd.regs[0x02>>1].byte.l & ~0xc0) | (data & 0xc0);
            return;
          }

          case 0x0e: /* MAIN-CPU communication flags */
          case 0x0f: /* !LWR is ignored (Space Ace, Dragon's Lair) */
          {
            m68k_poll_sync(1<<0x0e);
            scd.regs[0x0e>>1].byte.h = data;
            return;
          }

          default:
          {
            /* MAIN-CPU communication words */
            if ((address & 0x30) == 0x10)
            {
              m68k_poll_sync(1 << (address & 0x1f));

              /* register LSB */
              if (address & 1)
              {
                scd.regs[(address >> 1) & 0xff].byte.l = data;
                return;
              }

              /* register MSB */
              scd.regs[(address >> 1) & 0xff].byte.h = data;
              return;
            }

            /* invalid address */
            m68k_unused_8_w(address, data);
            return;
          }
        }
      }

      m68k_unused_8_w(address, data);
      return;
    }

    case 0x30:  /* TIME */
    {
      cart.hw.time_w(address, data);
      return;
    }

    case 0x41:  /* BOOT ROM */
    {
      if ((config.bios & 1) && (address & 1))
      {
        gen_bankswitch_w(data & 1);
        return;
      }
      m68k_unused_8_w(address, data);
      return;
    }

    case 0x10:  /* MEMORY MODE */
    case 0x13:  /* unknown */
    case 0x40:  /* TMSS */
    case 0x44:  /* RADICA */
    case 0x50:  /* SVP */
    {
      m68k_unused_8_w(address, data);
      return;
    }

    default:  /* Invalid address */
    {
      m68k_lockup_w_8(address, data);
      return;
    }
  }
}

void ctrl_io_write_word(unsigned int address, unsigned int data)
{
  switch ((address >> 8) & 0xFF)
  {
    case 0x00:  /* I/O chip */
    {
      if (!(address & 0xE0))
      {
        io_68k_write((address >> 1) & 0x0F, data & 0xFF);
        return;
      }
      m68k_unused_16_w(address, data);
      return;
    }

    case 0x11:  /* Z80 BUSREQ */
    {
      gen_zbusreq_w((data >> 8) & 1, m68k.cycles);
      return;
    }

    case 0x12:  /* Z80 RESET */
    {
      gen_zreset_w((data >> 8) & 1, m68k.cycles);
      return;
    }

    case 0x20:  /* MEGA-CD */
    {
#ifdef LOG_SCD
      error("[%d][%d]write word CD register %X -> 0x%04X (%X)\n", v_counter, m68k.cycles, address, data, m68k.pc);
#endif
      if (system_hw == SYSTEM_MCD)
      {
        /* register index ($A12000-A1203F mirrored up to $A120FF) */
        switch (address & 0x3e)
        {
          case 0x00:  /* SUB-CPU interrupt & control */
          {
            unsigned int halted = s68k.stopped;

            /* RESET bit */
            if (data & 0x01)
            {
              /* SUB-CPU reset is triggered on /RESET input 0->1 transition */
              if (!(scd.regs[0x00].byte.l & 0x01))
              {
                s68k_pulse_reset();
              }

              /* BUSREQ bit */
              if (data & 0x02)
              {
                /* SUB-CPU is halted (/HALT input is asserted) */
                s68k_pulse_halt();
              }
              else
              {
                /* SUB-CPU is running (/HALT input is released) */
                s68k_clear_halt();
              }

              /* update BUSREQ and RESET bits */
              scd.regs[0x00].byte.l = data & 0x03;
            }
            else
            {
              /* SUB-CPU is halted (/HALT and /RESET inputs are asserted) */
              s68k_pulse_halt();

              /* RESET bit is cleared and BUSREQ bit is set to 1 (verified on real hardware) */
              scd.regs[0x00].byte.l = 0x02;
            }

            /* BUSREQ bit remains set to 0 if SUB-CPU is halted while stopped (verified on real hardware) */
            if (s68k.stopped & 0x01)
            {
              scd.regs[0x00].byte.l &= ~0x02;
            }

            /* check if SUB-CPU halt status has changed */
            if (s68k.stopped != halted)
            {
              /* PRG-RAM (128KB bank) is normally mapped to $020000-$03FFFF (resp. $420000-$43FFFF) */
              unsigned int base = scd.cartridge.boot + 0x02;

              /* PRG-RAM can only be accessed from MAIN-CPU & Z80 when BUSREQ bit is set (Dungeon Explorer USA version) */
              if (scd.regs[0x00].byte.l & 0x02)
              {
                m68k.memory_map[base].read8   = m68k.memory_map[base+1].read8   = NULL;
                m68k.memory_map[base].read16  = m68k.memory_map[base+1].read16  = NULL;
                m68k.memory_map[base].write8  = m68k.memory_map[base+1].write8  = NULL;
                m68k.memory_map[base].write16 = m68k.memory_map[base+1].write16 = NULL;
                zbank_memory_map[base].read   = zbank_memory_map[base+1].read   = NULL;
                zbank_memory_map[base].write  = zbank_memory_map[base+1].write  = NULL;

                /* check if CDC DMA to PRG-RAM is running */
                if (cdc.dma_w == prg_ram_dma_w)
                {
                  /* synchronize CDC DMA with MAIN-CPU */
                  cdc_dma_update((m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE);

                  /* halt CDC DMA to PRG-RAM (if still running) */
                  cdc.halted_dma_w = cdc.dma_w;
                  cdc.dma_w = 0;
                }
              }
              else
              {
                m68k.memory_map[base].read8   = m68k.memory_map[base+1].read8   = m68k_read_bus_8;
                m68k.memory_map[base].read16  = m68k.memory_map[base+1].read16  = m68k_read_bus_16;
                m68k.memory_map[base].write8  = m68k.memory_map[base+1].write8  = m68k_unused_8_w;
                m68k.memory_map[base].write16 = m68k.memory_map[base+1].write16 = m68k_unused_16_w;
                zbank_memory_map[base].read   = zbank_memory_map[base+1].read   = zbank_unused_r;
                zbank_memory_map[base].write  = zbank_memory_map[base+1].write  = zbank_unused_w;

                /* check if CDC DMA to PRG-RAM is halted */
                if (cdc.halted_dma_w == prg_ram_dma_w)
                {
                  /* relative SUB-CPU cycle counter */
                  unsigned int cycles = (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE;

                  /* enable CDC DMA to PRG-RAM */
                  cdc.dma_w = prg_ram_dma_w;
                  cdc.halted_dma_w = 0;

                  /* synchronize CDC DMA with MAIN-CPU  (only if not already ahead) */
                  if (cdc.cycles[0] < cycles)
                  {
                    cdc.cycles[0] = cycles;
                  }
                }
              }
            }

            /* IFL2 bit */
            if (data & 0x100)
            {
              /* level 2 interrupt enabled ? */
              if (scd.regs[0x32>>1].byte.l & 0x04)
              {
                /* set IFL2 flag */
                scd.regs[0x00].byte.h |= 0x01;

                /* trigger level 2 interrupt */
                scd.pending |= (1 << 2);

                /* update IRQ level */
                s68k_update_irq((scd.pending & scd.regs[0x32>>1].byte.l) >> 1);
              }
            }
            return;
          }

          case 0x02:  /* Memory Mode */
          {
            m68k_poll_sync(1<<0x03);

            /* PRG-RAM 128k bank mapped to $020000-$03FFFF (resp. $420000-$43FFFF) */
            m68k.memory_map[scd.cartridge.boot + 0x02].base = scd.prg_ram + ((data & 0xc0) << 11);
            m68k.memory_map[scd.cartridge.boot + 0x03].base = m68k.memory_map[scd.cartridge.boot + 0x02].base + 0x10000;

            /* check current mode */
            if (scd.regs[0x03>>1].byte.l & 0x04)
            {
              /* DMNA bit */
              if (data & 0x02)
              {
                /* writing 1 to DMNA in 1M mode will return Word-RAM to SUB-CPU in 2M mode */
                scd.dmna = 1;
              }
              else
              {
                /* writing 0 to DMNA in 1M mode actually sets DMNA bit */
                data |= 0x02;

                /* update WP0-7, BK0-1 & DMNA bits */
                scd.regs[0x02>>1].w = (scd.regs[0x02>>1].w & ~0xffc2) | (data & 0xffc2);
                return;
              }
            }
            else
            {
              /* writing 0 to DMNA in 2M mode does nothing */
              if (data & 0x02)
              {
                int i;

                /* Word-RAM is assigned to SUB-CPU */
                scd.dmna = 1;

                /* MAIN-CPU: $200000-$23FFFF is unmapped */
                for (i=scd.cartridge.boot+0x20; i<scd.cartridge.boot+0x24; i++)
                {
                  m68k.memory_map[i].read8   = m68k_read_bus_8;
                  m68k.memory_map[i].read16  = m68k_read_bus_16;
                  m68k.memory_map[i].write8  = m68k_unused_8_w;
                  m68k.memory_map[i].write16 = m68k_unused_16_w;
                  zbank_memory_map[i].read   = zbank_unused_r;
                  zbank_memory_map[i].write  = zbank_unused_w;
                }

                /* SUB-CPU: access to Word-RAM at 0x080000-0x0BFFFF is unlocked (/DTACK asserted) */
                for (i=0x08; i<0x0c; i++)
                {
                  s68k.memory_map[i].read8   = NULL;
                  s68k.memory_map[i].read16  = NULL;
                  s68k.memory_map[i].write8  = NULL;
                  s68k.memory_map[i].write16 = NULL;
                }

                /* clear RET bit and update WP0-7 & BK0-1 bits */
                scd.regs[0x02>>1].w = (scd.regs[0x02>>1].w & ~0xffc3) | (data & 0xffc2);

                /* check if SUB-CPU is waiting for Word-RAM access */
                if (s68k.stopped & 0x04)
                {
                  /* synchronize SUB-CPU with MAIN-CPU */
                  s68k.cycles = (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE;

                  /* restart SUB-CPU */
                  s68k_clear_wait();
#ifdef LOG_SCD
                  error("s68k started from %d cycles\n", s68k.cycles);
#endif
                }

                /* check if graphics operation is started */
                if (scd.regs[0x58>>1].byte.h & 0x80)
                {
                  /* relative SUB-CPU cycle counter */
                  unsigned int cycles = (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE;

                  /* synchronize GFX processing with MAIN-CPU (only if not already ahead) */
                  if (gfx.cycles < cycles)
                  {
                    gfx.cycles = cycles;
                  }
                }

                /* check if CDC DMA to 2M Word-RAM is halted */
                if (cdc.halted_dma_w == word_ram_2M_dma_w)
                {
                  /* relative SUB-CPU cycle counter */
                  unsigned int cycles = (m68k.cycles * SCYCLES_PER_LINE) / MCYCLES_PER_LINE;

                  /* enable CDC DMA to 2M Word-RAM */
                  cdc.dma_w = word_ram_2M_dma_w;
                  cdc.halted_dma_w = 0;

                  /* synchronize CDC DMA with MAIN-CPU (only if not already ahead) */
                  if (cdc.cycles[0] < cycles)
                  {
                    cdc.cycles[0] = cycles;
                  }
                }
                return;
              }
            }

            /* update WP0-7 & BK0-1 bits only */
            scd.regs[0x02>>1].w = (scd.regs[0x02>>1].w & ~0xffc0) | (data & 0xffc0);
            return;
          }

          case 0x06:  /* H-INT vector (word access only ?) */
          {
            *(uint16 *)(m68k.memory_map[scd.cartridge.boot].base + 0x72) = data;
            return;
          }

          case 0x08: /* CDC host data */
          {
            /* CDC data is also read (although unused) on write access (verified on real hardware, cf. Krikzz's mcd-verificator) */
            cdc_host_r(CDC_MAIN_CPU_ACCESS);
            return;
          }

          case 0x0e:  /* CPU communication flags */
          {
            m68k_poll_sync(1<<0x0e);

            /* D8-D15 ignored -> only MAIN-CPU flags are updated (Mortal Kombat) */
            scd.regs[0x0e>>1].byte.h = data & 0xff;
            return;
          }

          default:
          {
            /* MAIN-CPU communication words */
            if ((address & 0x30) == 0x10)
            {
              m68k_poll_sync(3 << (address & 0x1e));
              scd.regs[(address >> 1) & 0xff].w = data;
              return;
            }

            /* invalid address */
            m68k_unused_16_w (address, data);
            return;
          }
        }
      }

      m68k_unused_16_w (address, data);
      return;
    }

    case 0x30:  /* TIME */
    {
      cart.hw.time_w(address, data);
      return;
    }

    case 0x40:  /* TMSS */
    {
      if (config.bios & 1)
      {
        gen_tmss_w(address & 3, data);
        return;
      }
      m68k_unused_16_w(address, data);
      return;
    }

    case 0x50:  /* SVP */
    {
      if (!(address & 0xFD))
      {
        svp->ssp1601.gr[SSP_XST].byte.h = data;
        svp->ssp1601.gr[SSP_PM0].byte.h |= 2;
        svp->ssp1601.emu_status &= ~SSP_WAIT_PM0;
        return;
      }
      m68k_unused_16_w(address, data);
      return;
    }

    case 0x10:  /* MEMORY MODE */
    case 0x13:  /* unknown */
    case 0x41:  /* BOOT ROM */
    case 0x44:  /* RADICA */
    {
      m68k_unused_16_w (address, data);
      return;
    }
            
    default:  /* Invalid address */
    {
      m68k_lockup_w_16 (address, data);
      return;
    }
  }
}


/*--------------------------------------------------------------------------*/
/* VDP                                                                      */
/*--------------------------------------------------------------------------*/

unsigned int vdp_read_byte(unsigned int address)
{
  switch (address & 0xFD)
  {
    case 0x00:  /* DATA */
    {
      return (vdp_68k_data_r() >> 8);
    }

    case 0x01:  /* DATA */
    {
      return (vdp_68k_data_r() & 0xFF);
    }

    case 0x04:  /* CTRL */
    {
      unsigned int data = (vdp_68k_ctrl_r(m68k.cycles) >> 8) & 3;

      /* Unused bits return prefetched bus data */
      address = m68k.pc;
      data |= (READ_BYTE(m68k.memory_map[((address)>>16)&0xff].base, (address) & 0xffff) & 0xFC);

      return data;
    }

    case 0x05:  /* CTRL */
    {
      return (vdp_68k_ctrl_r(m68k.cycles) & 0xFF);
    }

    case 0x08:  /* HVC */
    case 0x0C:
    {
      return (vdp_hvc_r(m68k.cycles) >> 8);
    }

    case 0x09:  /* HVC */
    case 0x0D:
    {
      return (vdp_hvc_r(m68k.cycles) & 0xFF);
    }

    case 0x18:  /* Unused */
    case 0x19:
    case 0x1C:
    case 0x1D:
    {
      return m68k_read_bus_8(address);
    }

    default:    /* Invalid address */
    {
      return m68k_lockup_r_8(address);
    }
  }
}

unsigned int vdp_read_word(unsigned int address)
{
  switch (address & 0xFC)
  {
    case 0x00:  /* DATA */
    {
      return vdp_68k_data_r();
    }

    case 0x04:  /* CTRL */
    {
      unsigned int data = vdp_68k_ctrl_r(m68k.cycles) & 0x3FF;

      /* Unused bits return prefetched bus data */
      address = m68k.pc;
      data |= (*(uint16 *)(m68k.memory_map[((address)>>16)&0xff].base + ((address) & 0xffff)) & 0xFC00);

      return data;
    }

    case 0x08:  /* HVC */
    case 0x0C:
    {
      return vdp_hvc_r(m68k.cycles);
    }

    case 0x18:  /* Unused */
    case 0x1C:
    {
      return m68k_read_bus_16(address);
    }

    default:    /* Invalid address */
    {
      return m68k_lockup_r_16(address);
    }
  }
}

void vdp_write_byte(unsigned int address, unsigned int data)
{
  switch (address & 0xFC)
  {
    case 0x00:  /* Data port */
    {
      vdp_68k_data_w(data << 8 | data);
      return;
    }

    case 0x04:  /* Control port */
    {
      vdp_68k_ctrl_w(data << 8 | data);
      return;
    }

    case 0x10:  /* PSG */
    case 0x14:
    {
      if (address & 1)
      {
        psg_write(m68k.cycles, data);
        return;
      }
      m68k_unused_8_w(address, data);
      return;
    }

    case 0x18: /* Unused */
    {
      m68k_unused_8_w(address, data);
      return;
    }

    case 0x1C:  /* TEST register */
    {
      vdp_test_w(data << 8 | data);
      return;
    }

    default:  /* Invalid address */
    {
      m68k_lockup_w_8(address, data);
      return;
    }
  }
}

void vdp_write_word(unsigned int address, unsigned int data)
{
  switch (address & 0xFC)
  {
    case 0x00:  /* DATA */
    {
      vdp_68k_data_w(data);
      return;
    }

    case 0x04:  /* CTRL */
    {
      vdp_68k_ctrl_w(data);
      return;
    }

    case 0x10:  /* PSG */
    case 0x14:
    {
      psg_write(m68k.cycles, data & 0xFF);
      return;
    }

    case 0x18:  /* Unused */
    {
      m68k_unused_16_w(address, data);
      return;
    }
    
    case 0x1C:  /* Test register */
    {
      vdp_test_w(data);
      return;
    }

    default:  /* Invalid address */
    {
      m68k_lockup_w_16 (address, data);
      return;
    }
  }
}


/*--------------------------------------------------------------------------*/
/* PICO (incomplete)                                                        */
/*--------------------------------------------------------------------------*/

unsigned int pico_read_byte(unsigned int address)
{
  switch (address & 0xFF)
  {
    case 0x01:  /* VERSION register */
    {
      return (region_code >> 1);
    }

    case 0x03:  /* IO register */
    {
      return ~input.pad[0];
    }

    case 0x05:  /* PEN X coordinate (MSB) */
    {
      return (input.analog[0][0] >> 8);
    }

    case 0x07:  /* PEN X coordinate (LSB) */
    {
      return (input.analog[0][0] & 0xFF);
    }

    case 0x09:  /* PEN Y coordinate (MSB) */
    {
      return (input.analog[0][1] >> 8);
    }

    case 0x0B:  /* PEN Y coordinate (LSB) */
    {
      return (input.analog[0][1] & 0xFF);
    }

    case 0x0D:  /* PAGE register */
    {
      return (1 << pico_current) - 1;
    }

    case 0x10:  /* ADPCM data registers (TODO) */
    case 0x11:
    {
      return 0xff;
    }

    case 0x12:  /* ADPCM control registers (TODO) */
    {
      return 0x80;
    }

    default:
    {
      return m68k_read_bus_8(address);
    }
  }
}

unsigned int pico_read_word(unsigned int address)
{
  return (pico_read_byte(address | 1) | (pico_read_byte(address) << 8));
}
