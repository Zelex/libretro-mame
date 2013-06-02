#include "emu.h"

class pccard_interface
{
public:
	virtual DECLARE_READ16_MEMBER(read_memory);
	virtual DECLARE_READ16_MEMBER(read_reg);
	virtual DECLARE_WRITE16_MEMBER(write_memory);
	virtual DECLARE_WRITE16_MEMBER(write_reg);
};

extern const device_type PCCARD_SLOT;

class pccard_slot_device : public device_t,
	public device_slot_interface
{
public:
	pccard_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_READ_LINE_MEMBER(read_line_inserted);
	DECLARE_ADDRESS_MAP(memory, 16);
	DECLARE_ADDRESS_MAP(reg, 16);

protected:
	virtual void device_start();

private:
	DECLARE_READ16_MEMBER(read_memory);
	DECLARE_READ16_MEMBER(read_reg);
	DECLARE_WRITE16_MEMBER(write_memory);
	DECLARE_WRITE16_MEMBER(write_reg);

	// internal state
	pccard_interface *m_pccard;
};