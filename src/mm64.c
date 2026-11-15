/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(MM64)

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(addr_t *pte,
             int pre,       // present
             addr_t fpn,    // FPN
             int drt,       // dirty
             int swp,       // swap
             int swptyp,    // swap type
             addr_t swpoff) // swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0)
        return -1; // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    } else { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}

/*
 * get_pd_from_address - Parse address to 5 page directory level
 * @addr  : address
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table
 */
int get_pd_from_address(addr_t addr, addr_t *pgd, addr_t *p4d, addr_t *pud,
                        addr_t *pmd, addr_t *pt) {
  /* Check if the adress is canonical */
  uint64_t unused = addr >> (PAGING64_ADDR_PGD_HIBIT + 1);
  uint64_t unused_ones = (uint64_t)(-1) >> (PAGING64_ADDR_PGD_HIBIT + 1);
  if (unused != 0 && unused != unused_ones) {
    printf("[ERROR %s] address passed is not under canonical form\n", __func__);
    return 1;
  }

  /* Extract page directories */
  *pgd = (addr & PAGING64_ADDR_PGD_MASK) >> PAGING64_ADDR_PGD_LOBIT;
  *p4d = (addr & PAGING64_ADDR_P4D_MASK) >> PAGING64_ADDR_P4D_LOBIT;
  *pud = (addr & PAGING64_ADDR_PUD_MASK) >> PAGING64_ADDR_PUD_LOBIT;
  *pmd = (addr & PAGING64_ADDR_PMD_MASK) >> PAGING64_ADDR_PMD_LOBIT;
  *pt = (addr & PAGING64_ADDR_PT_MASK) >> PAGING64_ADDR_PT_LOBIT;

  /* TODO: implement the page directories mapping */
  return 0;
}

/*
 * get_pd_from_pagenum - Parse page number to 5 page directory level
 * @pgn   : page number
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table
 */
int get_pd_from_pagenum(addr_t pgn, addr_t *pgd, addr_t *p4d, addr_t *pud,
                        addr_t *pmd, addr_t *pt) {
  /* Shift the address to get page num and perform the mapping*/
  return get_pd_from_address(pgn << PAGING64_ADDR_PT_SHIFT, pgd, p4d, pud, pmd,
                             pt);
}

// get the pgn and allocate the pages when not present
static addr_t *pt_walk(struct mm_struct *mm, addr_t pgn, int alloc) {
  addr_t idx[5]; // PGD, P4D, PUD, PMD, PT
  if (get_pd_from_pagenum(pgn, &idx[0], &idx[1], &idx[2], &idx[3], &idx[4]) !=
      0) {
    return NULL;
  }

  const int pt_entries =
      1 << (PAGING64_ADDR_PGD_HIBIT - PAGING64_ADDR_PGD_LOBIT + 1);
  addr_t *table = mm->pgd;
  if (table == NULL) {
    return NULL;
  }

  for (int i = 0; i < 4; i++) {
    addr_t *slot = &table[idx[i]];
    if (*slot == 0) { // table not allocated
      if (!alloc) {
        return NULL;
      }
      *slot = (addr_t)calloc(pt_entries, sizeof(addr_t));
      if (*slot == 0) {
        fprintf(stderr, "calloc failed");
        return NULL;
      }
    }
    table = (addr_t *)*slot;
  }

  return &table[idx[4]];
}

/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff) {
  struct krnl_t *krnl = caller->krnl;

  addr_t *pte;
  addr_t pgd = 0;
  addr_t p4d = 0;
  addr_t pud = 0;
  addr_t pmd = 0;
  addr_t pt = 0;

  // dummy pte alloc to avoid runtime error
  // pte = malloc(sizeof(addr_t));
#ifdef MM64
  /* Get value from the system */
  /* TODO Perform multi-level page mapping */
  // get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);
  pte = pt_walk(krnl->mm, pgn, 1);
  //... krnl->mm->pgd
  //... krnl->mm->pt
  // pte = &krnl->mm->pt;
#else
  pte = &krnl->mm->pgd[pgn];
#endif
  if (pte == NULL) {
    return 1;
  }

  CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 * @caller: caller
 */
int pte_set_fpn(struct mm_struct *mm, addr_t pgn, addr_t fpn,
                struct pcb_t *caller) {
  addr_t *pte = NULL;

#ifdef MM64
  pte = pt_walk(mm, pgn, 1);
#else
  pte = &mm->pgd[pgn];
#endif
  if (pte == NULL) {
    return 1;
  }
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}

/* Get PTE page table entry
 * @mm     : mm
 * @pgn    : page number
 * @caller : caller
 * @ret    : page table entry
 **/
addr_t pte_get_entry(struct mm_struct *mm, addr_t pgn, struct pcb_t *caller) {
  addr_t *pte = NULL;

#ifdef MM64
  pte = pt_walk(mm, pgn, 0);
#else
  pte = &mm->pgd[pgn];
#endif

  if (pte == NULL) {
    return 0;
  }
  return (addr_t)*pte;
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct mm_struct *mm, addr_t pgn, uint64_t pte_val,
                  struct pcb_t *caller) {
  addr_t *pte = NULL;
#ifdef MM64
  pte = pt_walk(mm, pgn, 1);
#else
  pte = &mm->pgd[pgn];
#endif
  if (pte == NULL) {
    return 1;
  }

  *pte = pte_val;

  return 0;
}

/*
 * vmap_pgd_memset - memset the pages with given pattern
 * If the page is not allocated, skip it.
 */
int vmap_pgd_memset(struct pcb_t *caller, // process call
                    addr_t addr, // start address which is aligned to pagesz
                    int pgnum)   // num of mapping page
{
  uint64_t pattern = 0xdeadbeef;
  struct mm_struct *mm = caller->mm;
  struct memphy_struct *ram = caller->krnl->mram;
  addr_t pgn = addr >> PAGING64_ADDR_PT_SHIFT;

  // memset the page table with given pattern
  for (int i = 0; i < pgnum; i++) {
    addr_t *pte = pt_walk(mm, pgn + i, 0);
    if (pte == NULL || !(*pte & PAGING_PTE_PRESENT_MASK))
      continue; // page not mapped in ram

    addr_t fpn = GETVAL(*pte, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    for (int b = 0; b < PAGING64_PAGESZ; b++) {
      BYTE v = (pattern >> (8 * (b % 4))) &
               0xFF; /* byte-wise writing to avoid big/small endian issues */
      MEMPHY_write(ram, fpn * PAGING64_PAGESZ + b, v);
    }
  }

  return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(
    struct pcb_t *caller, // process call
    addr_t addr,          // start address which is aligned to pagesz // start
    int pgnum,            // num of mapping page
    struct framephy_struct *frames, // list of the mapped frames
    struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{                                   // no guarantee all given pages are mapped
  struct framephy_struct *fpit;
  int pgit;
  addr_t pgn = addr >> PAGING64_ADDR_PT_SHIFT;

  /* TODO: update the rg_end and rg_start of ret_rg */
  ret_rg->rg_start = addr;
  ret_rg->rg_end = addr + pgnum * PAGING64_PAGESZ;
  // ret_rg->vmaid = ...

  // map range of frame to address space
  struct mm_struct *mm = caller->mm;
  for (pgit = 0, fpit = frames; pgit < pgnum && fpit != NULL;
       pgit++, fpit = fpit->fp_next) {
    if (pte_set_fpn(mm, pgn + pgit, fpit->fpn, caller) != 0) // point the page
      return 1;
  }

  /* Tracking for later page replacement activities (if needed)
   * Enqueue new usage page */
  // enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn64 + pgit);

  return 0;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : return frame list
 */
addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum,
                         struct framephy_struct **frm_lst) {
  struct framephy_struct *newfp = NULL;
  struct framephy_struct **cur = frm_lst;

  // allocate the page
  struct mm_struct *mm = caller->mm;
  struct memphy_struct *ram = caller->krnl->mram;

  addr_t fpn;
  int pgit;
  enum mm64_error err_code = 0;
  for (pgit = 0; pgit < req_pgnum; pgit++) {
    // allocate the page
    if (MEMPHY_get_freefp(ram, &fpn) == 0) {
      newfp = (struct framephy_struct *)malloc(sizeof(*newfp));
      if (newfp == NULL) {
        err_code = ERR_ALLOC_UNK;
        break;
      }
      newfp->fpn = fpn;
      newfp->fp_next = NULL;
      newfp->owner = mm;

      *cur = newfp;
      cur = &newfp->fp_next;
    } else { // obtaining some but not enough frames
      err_code = ERR_ALLOC_NO_MEM;
      break;
    }
  }

  // in case of an error, we free already allocated pages
  if (pgit != req_pgnum) {
    struct framephy_struct *node = *frm_lst;
    while (node != NULL) {
      struct framephy_struct *next = node->fp_next; /* save before free */
      MEMPHY_put_freefp(ram, node->fpn);            /* return frame to RAM */
      free(node);
      node = next;
    }
    *frm_lst = NULL; /* no dangling list for the caller */
    return err_code;
  }

  return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend,
                  addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg) {
  struct framephy_struct *frm_lst = NULL;
  addr_t ret_alloc = 0;

  /*@bksysnet: author provides a feasible solution of getting frames
   *FATAL logic in here, wrong behaviour if we have not enough page
   *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
   *Don't try to perform that case in this simple work, it will result
   *in endless procedure of swap-off to get frame and we have not provide
   *duplicate control mechanism, keep it simple
   */
  ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);

  if (ret_alloc != 0 && ret_alloc != ERR_ALLOC_NO_MEM)
    return -1;

  /* Out of memory */
  if (ret_alloc == ERR_ALLOC_NO_MEM) {
    return -1;
  }

  /* it leaves the case of memory is enough but half in ram, half in swap
   * do the swaping all to swapper to get the all in ram */
  vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

  return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
                   struct memphy_struct *mpdst, addr_t dstfpn) {
  int cellidx;
  addr_t addrsrc, addrdst;
#ifdef MM64
  for (cellidx = 0; cellidx < PAGING64_PAGESZ; cellidx++) {
    addrsrc = srcfpn * PAGING64_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING64_PAGESZ + cellidx;
#else
  for (cellidx = 0; cellidx < PAGING64_PAGESZ; cellidx++) {
    addrsrc = srcfpn * PAGING64_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING64_PAGESZ + cellidx;
#endif

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }

  return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller) {
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
  if (vma0 == NULL)
    return -1;

  mm->pgd = calloc(1 << (PAGING64_ADDR_PGD_HIBIT - PAGING64_ADDR_PGD_LOBIT + 1),
                   sizeof(uint64_t));
  if (mm->pgd == NULL) {
    free(vma0);
    return -1;
  }
  /* Lower levels are created on demand during the walk, so these start as NULL.
   */
  mm->p4d = NULL;
  mm->pud = NULL;
  mm->pmd = NULL;
  mm->pt = NULL;

  // first VMA (heap/data segment)
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;
  vma0->vm_freerg_list = NULL;
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  vma0->vm_next = NULL; /* only one VMA for now */
  vma0->vm_mm = mm;     /* point back to the owner */

  // Memory map bookkeeping
  mm->mmap = vma0;
  memset(mm->symrgtbl, 0, sizeof(mm->symrgtbl)); /* all variables unused */
  mm->fifo_pgn = NULL;                           /* no pages tracked yet */
  mm->kcpooltbl = NULL;                          /* no kernel cache pool yet */

  return 0;
}

struct vm_rg_struct *init_vm_rg(addr_t rg_start, addr_t rg_end) {
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist,
                      struct vm_rg_struct *rgnode) {
  rgnode->rg_next = *rglist;
  *rglist = rgnode;

  return 0;
}

int enlist_pgn_node(struct pgn_t **plist, addr_t pgn) {
  struct pgn_t *pnode = malloc(sizeof(struct pgn_t));

  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;

  return 0;
}

int print_list_fp(struct framephy_struct *ifp) {
  struct framephy_struct *fp = ifp;

  printf("print_list_fp: ");
  if (fp == NULL) {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (fp != NULL) {
    printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct *irg) {
  struct vm_rg_struct *rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL) {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (rg != NULL) {
    printf("rg[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

int print_list_vma(struct vm_area_struct *ivma) {
  struct vm_area_struct *vma = ivma;

  printf("print_list_vma: ");
  if (vma == NULL) {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (vma != NULL) {
    printf("va[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", vma->vm_start,
           vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

int print_list_pgn(struct pgn_t *ip) {
  printf("print_list_pgn: ");
  if (ip == NULL) {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (ip != NULL) {
    printf("va[" FORMAT_ADDR "]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("\n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end) {
  // addr_t pgn = ...
  // addr_t pgit = ...
  // struct mm_struct *mm = ...
  addr_t pgd = 0;
  addr_t p4d = 0;
  addr_t pud = 0;
  addr_t pmd = 0;
  addr_t pt = 0;

  get_pd_from_address(start, &pgd, &p4d, &pud, &pmd, &pt);

  /* TODO traverse the page map and dump the page directory entries */

  return 0;
}

#endif // def MM64
