c
c  Copyright (c) 2000-2007, Stanford University, 
c     Rensselaer Polytechnic Institute, Kenneth E. Jansen, 
c     Charles A. Taylor (see SimVascular Acknowledgements file 
c     for additional contributors to the source code).
c
c  All rights reserved.
c
c  Redistribution and use in source and binary forms, with or without 
c  modification, are permitted provided that the following conditions 
c  are met:
c
c  Redistributions of source code must retain the above copyright notice,
c  this list of conditions and the following disclaimer. 
c  Redistributions in binary form must reproduce the above copyright 
c  notice, this list of conditions and the following disclaimer in the 
c  documentation and/or other materials provided with the distribution. 
c  Neither the name of the Stanford University or Rensselaer Polytechnic
c  Institute nor the names of its contributors may be used to endorse or
c  promote products derived from this software without specific prior 
c  written permission.
c
c  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
c  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
c  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
c  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
c  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
c  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
c  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
c  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
c  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
c  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
c  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
c  DAMAGE.
c
c
      subroutine stannspcg(n,ndim,coef,jcoef1,jcoef2,rhs,u)
c
c ... array declarations.
c
c      implicit double precision (a-h, o-z)
      implicit none
      integer n,ndim,jcoef1(1),jcoef2(1)
      double precision coef(1),rhs(1),u(1)
c
c      double precision, allocatable :: u(:) 
      double precision, allocatable :: wksp(:)
      double precision ubar(1), rparm(30)
      integer, allocatable :: jcoef(:,:)
      integer, allocatable :: iwksp(:) 
      integer iparm(30), p(1), ip(1)
      integer mdim,maxnz,nws,i,j,ier
c
      integer itmax,nwa,nwp,inwp,nwf,inwf,nw,inw
c
      external cg, lsp4
c
c  Dymanically allocate memory    
c
c
c      do while (0 .eq. 0)
c      enddo 
      mdim = 2
      maxnz = ndim
      allocate(jcoef(ndim,2))
c      allocate(u(n))
c  Size of workspace for cg + lsp4 
      itmax = 9999
      nwa = 3*n+2*itmax
      nwp = 2*n
      inwp = 0
      nwf = 0
      inwf = 0
      nws = 0
      nw = nwa + nws + nwp + nwf
      inw = inwp + inwf + 3*n
c
c      write(*,'(A,I)') 'N=',n
c      write(*,'(A,I)') 'ndim=',ndim
c      write(*,'(A,I)') 'maxnz=',maxnz
c      do i=1,ndim
c        write(*,'(A,I10,F30.5)') 'f_coef ',i,coef(i)
c      enddo
c
c  Create two-dimensional Fortran-style array jcoef
c   (symmetric format)
c
      do i = 1,ndim
         jcoef(i,1) = jcoef1(i)
         jcoef(i,2) = jcoef2(i)
      enddo
c
c      do i=1,ndim
c        do j=1,2
c          write(*,'(A,I,I,I)') 'jcoef ',i,j,jcoef(i,j)
c        enddo
c      enddo
c
      allocate(wksp(nw))
      allocate(iwksp(inw))
c
      call dfault (iparm,rparm)
c
c ... now, reset some default values.
c
c ... specify symmetric coordinate format
      iparm(12) = 4
c ... specify ntest 5 instead of default 2 
      iparm(1) = 5
c
      iparm(2) = itmax 
      iparm(3) = 3
      rparm(1) = 1.0d-08
c
c ... generate an initial guess for u and call nspcg.
c
      call vfill (n,u,0.0d0)
c
c      write(*,'(A,I)') 'before call nw=',nw
c      write(*,'(A,I)') 'before call inw=',inw   
      call nspcg (lsp4,cg,ndim,mdim,n,maxnz,coef,jcoef,p,ip,
     a            u,ubar,rhs,wksp,iwksp,nw,inw,iparm,rparm,ier)
c  Clean up 
c      write(*,'(A,I)') 'after call nw=',nw
c      write(*,'(A,I)') 'after call inw=',inw 
      deallocate(wksp)
      deallocate(iwksp)
c
      allocate(wksp(nw))
      allocate(iwksp(inw))
c      write(*,'(A,I)') 'before 2nd call nw=',nw
c      write(*,'(A,I)') 'before 2nd call inw=',inw   
      call nspcg (lsp4,cg,ndim,mdim,n,maxnz,coef,jcoef,p,ip,
     a            u,ubar,rhs,wksp,iwksp,nw,inw,iparm,rparm,ier)
c  Clean up 
c      write(*,'(A,I)') 'after 2nd call nw=',nw
c      write(*,'(A,I)') 'after 2nd call inw=',inw 
      deallocate(wksp)
      deallocate(iwksp)      
      deallocate(jcoef)
c
      return
      end
