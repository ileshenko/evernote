#include "ibmuftian_utils.h"
#include "ibmuftian_bdmgr.h"
#include "ibmuftian_ioctl.h"
#include "ibmuftian.h"

unsigned int power(int a,unsigned int n)
{
    unsigned int ans=1;
    for(;n>0;n--)
    {
        ans*=a;
    }
    return ans;
}

int set_kernel_pages(void *uaddr,  void * uaddr_end, int nr,struct page **pages)
{
	int i = 0;
	struct page *pg;
	for (pg = virt_to_page(uaddr); pg <= virt_to_page(uaddr_end - 1); pg++)
    {
	   pages[i] = pg;
	   i++;
	}

	return i;
}

int map_user_pages(struct sgt_container **sgtc, unsigned long uaddr1, unsigned long uaddr_end1, enum dma_data_direction direction, unsigned long size_per_sg, int* isMalloc, unsigned int *total_sg_num,struct page ***pages, int *page_num, void **kernel_ptr)
{
	int res=0, j,k;
	unsigned long ofs;
	size_t avail, len;
	int nr_scatter;
	int sg_nums;
	int sg_iter=0;
	int const write_access = (direction == DMA_FROM_DEVICE || direction == DMA_BIDIRECTIONAL);
	int pages_per_sg;
	struct scatterlist *sg = NULL;
	unsigned long uaddr;
	unsigned long uaddr_end;
	unsigned long ttlength;
	unsigned int order=0;
	
	if(uaddr1==0 || uaddr1==uaddr_end1){
		*total_sg_num=0;
		return 0;
	}	
	
	pages_per_sg=size_per_sg/PAGE_SIZE;
	ttlength=uaddr_end1-uaddr1;

	
	//does it need to be filled in to continous memory?
	if(*total_sg_num==1 && (uaddr1>> PAGE_SHIFT) != (uaddr_end1>> PAGE_SHIFT)){
		*isMalloc=1;
	}
	else
		*isMalloc=0;
	
	if(*isMalloc){
		int rc;	
		order=get_order(ttlength);
		
		PRINT(DBG_PRAM({MMAP_DBG}), "get pages by order %d ...",order);
		*kernel_ptr=(void *)__get_free_pages(GFP_KERNEL,order);
		PRINT(DBG_PRAM({MMAP_DBG}), "get free pages %p\n",*kernel_ptr);
		if (unlikely(*kernel_ptr == NULL)){
			PRINT(DBG_PRAM({MMAP_DBG, ERR_DBG}), "get free pages failed\n");
			return -1;
		}	

		*page_num=power(2,order);
		if(direction == DMA_TO_DEVICE){
			rc = copy_from_user(*kernel_ptr, (void __user *)uaddr1,ttlength);
			  
			if (unlikely(rc != 0))
			{	
			    PRINT(DBG_PRAM({MMAP_DBG, ERR_DBG}), "copy from user failed\n");
			    goto out_free_pages;
			}
		}
		else{
			//memset(*kernel_ptr,0,uaddr_end1-uaddr1);
		}

		uaddr=(unsigned long)(*kernel_ptr);
		uaddr_end=(unsigned long)(*kernel_ptr)+(uaddr_end1-uaddr1);
	}
	else{		
		uaddr=uaddr1;
		uaddr_end=uaddr_end1;
	}
	// compute number of pages (= number of scatterlist entries)
	{
	unsigned long const p0 = uaddr >> PAGE_SHIFT;
	unsigned long const p1 = (uaddr_end + PAGE_SIZE - 1) >> PAGE_SHIFT;
	nr_scatter = p1 - p0;
    if(uaddr%128)
    {
        unsigned long remaining_val = ttlength-((PAGE_SIZE*pages_per_sg)-(uaddr & ~PAGE_MASK)-(uaddr%128));
        int bad_allign_pgs = remaining_val/((pages_per_sg -1)*PAGE_SIZE) +  (remaining_val%((pages_per_sg-1)*PAGE_SIZE))?1:0;
        int last_bd_pages = ((remaining_val%((pages_per_sg-1)*PAGE_SIZE))/PAGE_SIZE) + (((remaining_val%((pages_per_sg-1)*PAGE_SIZE))%PAGE_SIZE)?1:0);
        int num_pages = pages_per_sg + (pages_per_sg *(remaining_val/((pages_per_sg -1)*PAGE_SIZE))) + bad_allign_pgs + last_bd_pages;
        nr_scatter = nr_scatter < num_pages ? num_pages: nr_scatter;

    }

	if(pages_per_sg==0)//calc by size
		sg_nums=(ttlength)/size_per_sg+((ttlength)%size_per_sg?1:0);
	else
		sg_nums=nr_scatter/pages_per_sg+(nr_scatter%pages_per_sg?1:0);
	
	}

	
	*sgtc = kmalloc(sg_nums * sizeof(**sgtc), GFP_KERNEL);
	if (unlikely(*sgtc == NULL))
		goto out_free_pages;
	
	
	for(sg_iter=0;sg_iter<sg_nums;sg_iter++){
		(*sgtc)[sg_iter].sgt= kmalloc(sizeof(struct sg_table), GFP_KERNEL);
		if (unlikely((*sgtc)[sg_iter].sgt == 0))
		{	
			PRINT(DBG_PRAM({MMAP_DBG, ERR_DBG}), "Malloc (*sgtc)[%d].sgt failed\n",sg_iter);
			goto out_free_sgt;
		}
		if(size_per_sg<=PAGE_SIZE) //
			res= sg_alloc_table((*sgtc)[sg_iter].sgt, 1, GFP_KERNEL);
		else if(sg_iter==sg_nums-1)
			res= sg_alloc_table((*sgtc)[sg_iter].sgt, nr_scatter-pages_per_sg*sg_iter, GFP_KERNEL);
		else
			res= sg_alloc_table((*sgtc)[sg_iter].sgt, pages_per_sg, GFP_KERNEL);
		if (unlikely(res != 0))
		{
			kfree((*sgtc)[sg_iter].sgt);
			PRINT(DBG_PRAM({MMAP_DBG, ERR_DBG}), "sg_alloc_table (*sgtc)[%d].sgt failed\n",sg_iter);
			goto out_free_sgt;
		}	
				
	}

	*pages = kmalloc(nr_scatter * sizeof(**pages), GFP_KERNEL);
	if (unlikely(*pages == NULL))
	{
		PRINT(DBG_PRAM({MMAP_DBG, ERR_DBG}), "Malloc *pages failed, nscatter %d\n",nr_scatter);
		goto out_free_sgt;
	}	
	
	if(*isMalloc){
		res = set_kernel_pages((void *)uaddr, (void *)uaddr_end, nr_scatter, *pages);
	}	
	else{
        if(nr_scatter < pages_per_sg)
		    res = get_user_pages_fast(uaddr & PAGE_MASK, nr_scatter, write_access, *pages);
        else
            res = get_user_pages_fast(uaddr & PAGE_MASK, pages_per_sg, write_access, *pages);

		*page_num=res;

		PRINT(DBG_PRAM({MMAP_DBG}), "get_user_pages_fast return %d, exp %d\n",res,nr_scatter);

		
	
	}	
	if (unlikely(res < (nr_scatter<pages_per_sg?nr_scatter:pages_per_sg))){
		goto out_unmap;
	}
	
	ofs = uaddr & ~PAGE_MASK;
	avail = PAGE_SIZE - ofs;
	len = uaddr_end - uaddr;
	if (len > avail)
		len = avail;
	
#ifdef CONFIG_NOT_COHERENT_CACHE
		__dma_sync_page((*pages)[0], ofs, len, direction);
#endif

   PRINT(DBG_PRAM({MMAP_DBG}), "page 0 is %p, ofs %lu, len %lu\n",(*pages)[0],ofs,len);
  
   for(j=ofs;j<ofs+len && j<ofs+256;j++)
       PRINT(DBG_PRAM({MMAP_DBG}), "User Byte%d: 0x%x\n", j, *(char*)((void *)(phys_to_virt(page_to_phys((*pages)[0]))) + j));
 

	j=0;
	(*sgtc)[0].sgt_offset=0;	
	if(size_per_sg<=PAGE_SIZE){
		int left=len;
		while(left>0){
			sg=(*sgtc)[j].sgt->sgl;		
			if(j==0){
#ifdef ALIGN_INSTQ_4K
				//align must be support by multi 4k instq
				left+=(ofs%size_per_sg);
				ofs-=(ofs%size_per_sg);
#endif
				(*sgtc)[0].sgt_len=size_per_sg;
				if((*sgtc)[0].sgt_len>left)
					(*sgtc)[0].sgt_len=left;

			}
			else{
				(*sgtc)[j].sgt_len=left>size_per_sg?size_per_sg:left;	
				(*sgtc)[j].sgt_offset=(*sgtc)[j-1].sgt_offset+(*sgtc)[j-1].sgt_len;				
			}	
			sg_set_page(sg, (*pages)[0], (*sgtc)[j].sgt_len, ofs);
			left-=(*sgtc)[j].sgt_len;
			ofs+=(*sgtc)[j].sgt_len;
			PRINT(DBG_PRAM({MMAP_DBG}), "sg %d offset is %lu, len is %lu\n",j,(*sgtc)[j].sgt_offset,(*sgtc)[j].sgt_len);
			j++;
			
		}
	}
	else{
		sg=(*sgtc)[0].sgt->sgl;		
		sg_set_page(sg, (*pages)[0], len, ofs);
		(*sgtc)[0].sgt_len=len;
		sg = sg_next(sg);
		PRINT(DBG_PRAM({MMAP_DBG}), "sg %d offset is %lu, len is %lu\n",j,(*sgtc)[j].sgt_offset,(*sgtc)[j].sgt_len);
	}		
	PRINT(DBG_PRAM({MMAP_DBG}), "mapped page 0 is %p\n",(*pages)[0]);
	uaddr += len;
	
	if (uaddr < uaddr_end)
	{
		int i = 1;
		while (1)
		{
				
			len = uaddr_end - uaddr;
			if (len > PAGE_SIZE)
				len = PAGE_SIZE;

#ifdef CONFIG_NOT_COHERENT_CACHE
			__dma_sync_page((*pages)[i], 0, len, direction);
#endif
			PRINT(DBG_PRAM({MMAP_DBG}), "page %d is %p,len is %lu\n",i,(*pages)[i],len);
			if(size_per_sg<=PAGE_SIZE){
				
				int left=len;
				ofs=0;
			
				while(left>0){
					sg=(*sgtc)[j].sgt->sgl;		
					(*sgtc)[j].sgt_len=left>size_per_sg?size_per_sg:left;
					if(j>0)
						(*sgtc)[j].sgt_offset=(*sgtc)[j-1].sgt_offset+(*sgtc)[j-1].sgt_len;
					sg_set_page(sg, (*pages)[i], (*sgtc)[j].sgt_len, ofs);
					left-=(*sgtc)[j].sgt_len;
					ofs+=(*sgtc)[j].sgt_len;
					PRINT(DBG_PRAM({MMAP_DBG}), "sg %d offset is %lu, len is %lu\n",j,(*sgtc)[j].sgt_offset,(*sgtc)[j].sgt_len);
					j++;												
				}
			}
			else{		
				
				if(i%pages_per_sg==0){ //if sgl is full of pages
					PRINT(DBG_PRAM({MMAP_DBG}), "sg %d offset is %lu, len is %lu\n",j,(*sgtc)[j].sgt_offset,(*sgtc)[j].sgt_len);
					j++;
					sg=(*sgtc)[j].sgt->sgl;	
					(*sgtc)[j].sgt_offset=(*sgtc)[j-1].sgt_offset+(*sgtc)[j-1].sgt_len;
					(*sgtc)[j].sgt_len=0;
                    if((nr_scatter - (pages_per_sg*j)) > pages_per_sg)
                        res = get_user_pages_fast(uaddr & PAGE_MASK, pages_per_sg, write_access, *pages+i);
                    else
                        res = get_user_pages_fast(uaddr & PAGE_MASK, (nr_scatter-(pages_per_sg*j)), write_access,
                        *pages+i);
                    *page_num += res;


		
				}	
			    if (i%pages_per_sg ==  (pages_per_sg - 1))
                {
                    int bad_offset = ((*sgtc)[j].sgt_len + len)%128;
                    if(bad_offset)
                    {
                            len = len - bad_offset;
                    }
                }
				ofs = uaddr & ~PAGE_MASK;
                avail = PAGE_SIZE - ofs;
                len = len > avail ? avail : len; 
                sg_set_page(sg, (*pages)[i], len, ofs);
                (*sgtc)[j].sgt_len+=len;
				
			}
			uaddr += len;
			if (uaddr >= uaddr_end)
				break;

			++i;
			if(size_per_sg>PAGE_SIZE)
				sg = sg_next(sg);
		}
	}
		
	
	
	*total_sg_num=sg_nums; //return for free
	
	if(size_per_sg>PAGE_SIZE)
		return j+1 ;
	else 
		return j;

out_unmap:

	if(!*isMalloc){

		if (res >= 0)
		{
			for (j = 0; j < res; j++)
				page_cache_release((*pages)[j]);
		
		}
	}
	
	kfree((*pages));
	
out_free_sgt:

	for(k = 0; k < sg_iter; k++)
    {
		sg_free_table((*sgtc)[k].sgt);
		kfree((*sgtc)[k].sgt);
	}
	
	kfree(*sgtc);

out_free_pages:
	
	if(*isMalloc)
    {
    	free_pages((unsigned long)(*kernel_ptr),order);
	}	

    return FAILURE;
}


void unmap_user_pages(struct sgt_container *sgl, int dirtied, int sg_num, int isMalloc, int total_sg_num, struct page **pages, int page_num, void* kernel_ptr)
{
	unsigned int i,j;
	
	if(total_sg_num==0)
		return;

	PRINT(DBG_PRAM({MMAP_DBG}), "un map user %d pages, total sg num %d\n",page_num,total_sg_num);
    if(isMalloc){
		free_pages((unsigned long)kernel_ptr,get_order(PAGE_SIZE*page_num));
		PRINT(DBG_PRAM({MMAP_DBG}), "free  pages addr  %p, page num %d\n",kernel_ptr, page_num);
	}
	else{
		for(i=0;i<page_num;i++){ //free sg which have pages
				
					
			// fixme-lspu should be: set_page_dirty_lock(page)
			if (dirtied && !PageReserved(pages[i])){
				PRINT(DBG_PRAM({MMAP_DBG}), "set page dirty %p\n",pages[i]);
				SetPageDirty(pages[i]);
				
			}

			PRINT(DBG_PRAM({MMAP_DBG}), "release  page  %p\n",pages[i]);
			page_cache_release(pages[i]); // fixme-lspu - use newer api put_page(page)
			
				
		}
	}

	kfree(pages);
	
	for(j=0;j<total_sg_num;j++){

		
		sg_free_table(sgl[j].sgt);
		kfree(sgl[j].sgt);
		
	}
	
	kfree(sgl);
}

//map user memory to single dma address
dma_addr_t map_user_memory(struct pci_dev* pdev, unsigned long uaddr, unsigned long uaddr_end, enum dma_data_direction direction, int *isMalloc, struct page ***pages, int *page_num, void **kernel_ptr)
{
	int res=0, i,j;
	unsigned long ofs;
	int nr_scatter;
	int const write_access = (direction == DMA_FROM_DEVICE || direction == DMA_BIDIRECTIONAL);
	unsigned long ttlength;
	dma_addr_t dma_addr;
    void * virt_addr;
	
	if(uaddr==0 || uaddr==uaddr_end){
		return 0;
	}	
	
	ttlength=uaddr_end-uaddr;

		
	// compute number of pages (= number of scatterlist entries)
	{
	unsigned long const p0 = uaddr >> PAGE_SHIFT;
	unsigned long const p1 = (uaddr_end + PAGE_SIZE - 1) >> PAGE_SHIFT;
	nr_scatter = p1 - p0;
	
	}
	
	*pages = kmalloc(nr_scatter * sizeof(**pages), GFP_KERNEL);
	if (unlikely(*pages == NULL))
	{
		PRINT(DBG_PRAM({MMAP_DBG, ERR_DBG}), "Malloc *pages failed, nscatter %d\n",nr_scatter);
		return -1;
	}	
	
	
	res = get_user_pages_fast(uaddr & PAGE_MASK, nr_scatter, write_access, *pages);
	*page_num=res;

	PRINT(DBG_PRAM({MMAP_DBG}), "get_user_pages_fast return %d, exp %d\n",res,nr_scatter);

	if (unlikely(res < nr_scatter))
    {
		PRINT(DBG_PRAM({MMAP_DBG, ERR_DBG}), "get_user_pages_fast failed, res %d nscatter %d\n",res,nr_scatter);
		goto out_unmap_memory;
	}

	for(i=1;i<nr_scatter;i++){
		if(page_to_pfn((*pages)[i])!=page_to_pfn((*pages)[i-1])+1){
			*isMalloc=1;
			break;
		}
	}	
	
	
	if(*isMalloc){

		int rc;	

		//unmap_user_memory(pdev, 0,direction,0,*pages,*page_num,kernel_ptr,ttlength);
		for(i=0;i<res;i++){ //free sg which have pages				
			
			PRINT(DBG_PRAM({MMAP_DBG}), "release  page  %p\n",pages[i]);
			page_cache_release((*pages)[i]); // fixme-lspu - use newer api put_page(page)
			
				
		}
		*kernel_ptr= dma_alloc_coherent(&pdev->dev, ttlength,&dma_addr,GFP_KERNEL | GFP_DMA32);
		if (unlikely(*kernel_ptr == NULL)){
			PRINT(DBG_PRAM({MMAP_DBG, ERR_DBG}), "dma alloc memory failed\n");
			return -1;
		}	
		PRINT(DBG_PRAM({MMAP_DBG}), "allocate dma addr %p, virt addr %p\n",(void *)dma_addr,*kernel_ptr);
		
		if(direction == DMA_TO_DEVICE){
			rc = copy_from_user(*kernel_ptr, (void __user *)uaddr,ttlength);
			  
			if (unlikely(rc != 0))
			{	
			    PRINT(DBG_PRAM({MMAP_DBG, ERR_DBG}), "copy from user failed\n");
				goto out_unmap_memory;
			}
		}
		
		return dma_addr;
	}

	else{
		ofs = uaddr & ~PAGE_MASK;
		virt_addr=phys_to_virt(page_to_phys((*pages)[0]));
		PRINT(DBG_PRAM({MMAP_DBG}), "kernel vaddr is %p, user is %lx\n",virt_addr+ofs,uaddr);
		*kernel_ptr=virt_addr+ofs;		
		return dma_map_single(&pdev->dev, virt_addr+ofs,ttlength,direction);

	}	

	

out_unmap_memory:

	if(*isMalloc){

		dma_free_coherent(&pdev->dev,  ttlength,kernel_ptr,dma_addr);
	}
	else{
		
		if (res >= 0)
		{
			for (j = 0; j < res; j++)
				page_cache_release((*pages)[j]);
		
		}
	}
	
	kfree((*pages));
	
	return -1;
}

void unmap_user_memory(struct pci_dev *pdev,dma_addr_t dma_addr, enum dma_data_direction direction,  int isMalloc,struct page **pages, int page_num,  void* kernel_ptr, unsigned long length )
{
	unsigned int i;

	if(page_num==0 || length==0)
		return;
	
	if(isMalloc)
    {
		dma_free_coherent(&pdev->dev,  length,kernel_ptr,dma_addr);
		PRINT(DBG_PRAM({MMAP_DBG}), "dma free addr  %p, page num %d\n",kernel_ptr, page_num);
	}
	else{
		dma_unmap_single(&pdev->dev,dma_addr,length,direction);
		for(i=0;i<page_num;i++)
        { //free sg which have pages
			// fixme-lspu should be: set_page_dirty_lock(page)
			if ((direction==DMA_FROM_DEVICE || direction == DMA_BIDIRECTIONAL )&& !PageReserved(pages[i]))
            {
				PRINT(DBG_PRAM({MMAP_DBG}), "set page dirty %p\n",pages[i]);
				SetPageDirty(pages[i]);
			}
			PRINT(DBG_PRAM({MMAP_DBG}), "release  page  %p\n",pages[i]);
			page_cache_release(pages[i]); // fixme-lspu - use newer api put_page(page)
		}
	}
	kfree(pages);
}


/****************************************************************************
 * Converts a C string to an unsigned long integer hiding the cumbersome
 * interface that strtoul() provides. Realize this is not like the glibc
 * version but the kernel implementation w/o error checks. Will return zero
 * if nothing can be converted. Defaults to base10 if it cannot be determined.
 *   */

unsigned long __cstr2ulong(const char *cstr)
{
    char *endptr;
    return simple_strtoul(cstr, &endptr, 0 /* auto-base detect */);

}

int trim_str(char *str)
{
    size_t len = 0;
    char *frontp = str;
    char *endp = NULL;

    if( str == NULL ) return -1;
    if( str[0] == '\0' ) return -1;

    len = strlen(str);
    endp = str + len;

    while(isspace((unsigned char) *frontp)) ++frontp;
    if(endp != frontp)
    {
        while(isspace((unsigned char) *(--endp)) && endp != frontp){}
    }

    if(str + len - 1 != endp)
        *(endp + 1) = '\0';
    else if(frontp != str &&  endp == frontp)
        *str = '\0';

    endp = str;
    if(frontp != str)
    {
        while(*frontp) *endp++ = *frontp++;
        *endp = '\0';
    }

    return 0;
}

ssize_t utilization_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    int length = 0, total_utilization[atomic_read(&ibmuftian.fpga_c->count)], total_avg_utilization = 0;
    long work_duration;
    ibmuftian_engn * engn_ptr = NULL;
    ibmuftian_fpga * fpga_ptr = NULL;
    struct timeval curr_time;

    memset(total_utilization, 0 , atomic_read(&ibmuftian.fpga_c->count) * sizeof(int));
    length += sprintf(buf, "Engn.No     Utilized Time(usec)     percentage Utilization\n");

    do_gettimeofday(&curr_time);

    ibmuftian.performance.total_time.tv_sec = curr_time.tv_sec - ibmuftian.performance.total_time.tv_sec;
    ibmuftian.performance.total_time.tv_usec = curr_time.tv_usec - ibmuftian.performance.total_time.tv_usec;

    work_duration = (1000000 * (ibmuftian.performance.total_time.tv_sec)) +  ibmuftian.performance.total_time.tv_usec;

    list_for_each_entry_reverse(engn_ptr, &(ibmuftian.engn_c->engn_l), list)
    {
        long time_utilization = (1000000 * (engn_ptr->engn_utilized.tv_sec)) +  engn_ptr->engn_utilized.tv_usec;

        int perc_utilization = (time_utilization * 10000)/work_duration;

        length += sprintf(buf + length, "   %d   ", engn_ptr->id + (engn_ptr->fpga->engns.engn_count * engn_ptr->fpga->id));
        length += sprintf(buf + length, "     %ld                       ", time_utilization);
        length += sprintf(buf + length, "%d.%02d\n", perc_utilization/100, perc_utilization%100);

        total_utilization[engn_ptr->fpga->id] += perc_utilization;
    }

    length += sprintf(buf + length, "\nFPGA.No     Avg percentage Utilization\n");

    list_for_each_entry_reverse(fpga_ptr, &(ibmuftian.fpga_c->fpga_l), list)
    {
        int avg_utilization = total_utilization[fpga_ptr->id]/fpga_ptr->engns.engn_count;

        length += sprintf(buf + length, "   %d        %d.%02d\n", fpga_ptr->id, avg_utilization/100 , avg_utilization%100);
        total_avg_utilization += avg_utilization;
    }

    total_avg_utilization = total_avg_utilization/(atomic_read(&ibmuftian.fpga_c->count));

    length += sprintf(buf + length, "\nTotal Avg percent Utilization\n");
    length += sprintf(buf + length, "         %d.%02d\n", total_avg_utilization/100, total_avg_utilization%100);

    return length;
}

ssize_t utilization_init(struct kobject * kobj, struct kobj_attribute * attr, const char * buf, size_t count)
{
    int input, ret;
    ibmuftian_engn * engn_ptr = NULL, *n_ptr = NULL;

    ret = kstrtoint(buf, 10, &input);

    if(input == 0)
    {
        list_for_each_entry_safe(engn_ptr, n_ptr, &(ibmuftian.engn_c->engn_l), list)
        {
            engn_ptr->duration.tv_sec = ZERO;
            engn_ptr->duration.tv_usec = ZERO;
            engn_ptr->engn_utilized.tv_sec = ZERO;
            engn_ptr->engn_utilized.tv_usec = ZERO;
        }
    }
    else
        PRINT(DBG_PRAM({ERR_DBG}), "Input 0 to clear the perf_attrs no other input valid");

    do_gettimeofday(&ibmuftian.performance.total_time);

    return (ssize_t)count;
}
