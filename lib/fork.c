// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!(err & FEC_WR) || !(uvpt[PGNUM(addr)] & PTE_COW))
		panic("pgfault: not a copy-on-write page");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	if ((r = sys_page_alloc(0, (void *) PFTEMP, PTE_U | PTE_W | PTE_P)) < 0)
		panic("sys_page_alloc: %e", r);
	
	memmove((void *) PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);

	if ((r = sys_page_map(0, (void *) PFTEMP, 0, ROUNDDOWN(addr, PGSIZE),
				PTE_U | PTE_W | PTE_P)) < 0)
		panic("sys_page_map: %e", r);

	if ((r = sys_page_unmap(0, (void *) PFTEMP)) < 0)
		panic("sys_page_unmap: %e", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r, perm;
	void *va;

	// LAB 4: Your code here.
	perm = PTE_U | PTE_P;
	if (uvpt[pn] & (PTE_COW | PTE_W))
		perm |= PTE_COW;

	va = (void *) (pn * PGSIZE);

	if ((r = sys_page_map(0, va, envid, va, perm)) < 0)
		return r;

	if ((uvpt[pn] & (PTE_COW | PTE_W)) && (r = sys_page_map(0, va, 0, va, perm)) < 0)
		return r;

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	extern void _pgfault_upcall(void);

	envid_t envid;
	int r;
	unsigned pn;
	void *va;

	set_pgfault_handler(pgfault);

	if ((envid = sys_exofork()) < 0)
		return envid;

	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (pn = 0; pn < PGNUM(UTOP); pn++) {
		if (!(uvpd[pn / NPTENTRIES] & PTE_P)) {
			pn += (NPTENTRIES - 1);
			continue;
		}

		if (!(uvpt[pn] & PTE_P))
			continue;
		
		va = (void *) (pn * PGSIZE);
		if (va == (void *) (UXSTACKTOP - PGSIZE)) {
			if ((r = sys_page_alloc(envid, va, PTE_U | PTE_W | PTE_P)) < 0)
				panic("sys_page_alloc: %e", r);
		} else {
			if ((r = duppage(envid, pn)) < 0)
				panic("duppage: %e", r);
		}
	}

	if ((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0)
		panic("sys_env_set_pgfault_upcall: %e", r);

	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
}

// Challenge!
int
sfork(void)
{
	extern void _pgfault_upcall(void);

	envid_t envid;
	int r;
	int perm;
	unsigned pn;
	void *va;

	set_pgfault_handler(pgfault);

	if ((envid = sys_exofork()) < 0)
		return envid;

	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (pn = 0; pn < PGNUM(UTOP); pn++) {
		if (!(uvpd[pn / NPTENTRIES] & PTE_P)) {
			pn += (NPTENTRIES - 1);
			continue;
		}

		perm = uvpt[pn] & PTE_SYSCALL;
		if (!(perm & PTE_P))
			continue;
		
		va = (void *) (pn * PGSIZE);
		if (va == (void *) (UXSTACKTOP - PGSIZE)) {
			if ((r = sys_page_alloc(envid, va, PTE_U | PTE_W | PTE_P)) < 0)
				panic("sys_page_alloc: %e", r);
		} else if (va == (void *) (USTACKTOP - PGSIZE)) {
			if ((r = duppage(envid, pn)) < 0)
				panic("duppage: %e", r);
		} else {
			if ((r = sys_page_map(0, va, envid, va, perm)) < 0)
				return r;
		}
	}

	if ((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0)
		panic("sys_env_set_pgfault_upcall: %e", r);

	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
}
