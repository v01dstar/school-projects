Documentation for Kernel Assignment 3
=====================================

+-------------+
| BUILD & RUN |
+-------------+

Comments:

Set "SHADOWD" = 1 in Config.mk

make

+-----------------+
| SKIP (Optional) |
+-----------------+

None

+---------+
| GRADING |
+---------+

(A.1) In mm/pframe.c:
    (a) In pframe_pin(): 1 out of 1 pt
    (b) In pframe_unpin(): 1 out of 1 pt

(A.2) In vm/mmap.c:
    (a) In do_mmap(): 2 out of 2 pts
    (b) In do_munmap(): 2 out of 2 pts

(A.3) In vm/vmmap.c:
    (a) In vmmap_destroy(): 2 out of 2 pts
    (b) In vmmap_insert(): 2 out of 2 pts
    (c) In vmmap_find_range(): 2 out of 2 pts
    (d) In vmmap_lookup(): 1 out of 1 pt
    (e) In vmmap_is_range_empty(): 1 out of 1 pt
    (f) In vmmap_map(): 7 out of 7 pts

(A.4) In vm/anon.c:
    (a) In anon_init(): 1 out of 1 pt
    (b) In anon_ref(): 1 out of 1 pt
    (c) In anon_put(): 1 out of 1 pt
    (d) In anon_fillpage(): 1 out of 1 pt

(A.5) In fs/vnode.c:
    (a) In special_file_mmap(): 2 out of 2 pts

(A.6) In vm/shadow.c:
    (a) In shadow_init(): 1 out of 1 pt
    (b) In shadow_ref(): 1 out of 1 pt
    (c) In shadow_put(): 1 out of 1 pts
    (d) In shadow_fillpage(): 2 out of 2 pts

(A.7) In proc/fork.c:
    (a) In do_fork(): 6 out of 6 pts

(A.8) In proc/kthread.c:
    (a) In kthread_clone(): 2 out of 2 pts

(B.1) /usr/bin/hello (3 out of 3 pts)
(B.2) /bin/uname -a (3 out of 3 pts)
(B.3) /usr/bin/args ab cde fghi j (3 out of 3 pts)
(B.4) /usr/bin/fork-and-wait (5 out of 5 pts)

(C.1) /usr/bin/segfault (1 out of 1 pt)

(D.2) /usr/bin/vfstest (7 out of 7 pts)
(D.3) /usr/bin/memtest (7 out of 7 pts)
(D.4) /usr/bin/eatmem (7 out of 7 pts)
(D.5) /usr/bin/forkbomb (7 out of 7 pts)
(D.6) /usr/bin/stress (7 out of 7 pts)

(E.1) /usr/bin/vfstest (1 out of 1 pt)
(E.2) /usr/bin/memtest (1 out of 1 pt)
(E.3) /usr/bin/eatmem (1 out of 1 pt)
(E.4) /usr/bin/forkbomb (1 out of 1 pt)
(E.5) /usr/bin/stress (1 out of 1 pt)

(F) Self-checks: (10 out of 10 pts)
    Comments: No need to do self-checks

Missing required section(s) in README file (vm-README.txt): 0
Submitted binary file : 0
Submitted extra (unmodified) file : 0
Wrong file location in submission : 0
Use dbg_print(...) instead of dbg(DBG_PRINT, ...) : 0
Not properly indentify which dbg() printout is for which item in the grading guidelines : 0
Cannot compile : 0
Compiler warnings : 0
"make clean" : 0
Useless KASSERT : 0
Insufficient/Confusing dbg : 0
Kernel panic : 0
Cannot halt kernel cleanly : 0

+------+
| BUGS |
+------+

Comments: No

+---------------------------+
| CONTRIBUTION FROM MEMBERS |
+---------------------------+

If not equal-share contribution, please list percentages.

+------------------+
| OTHER (Optional) |
+------------------+

Special DBG setting in Config.mk for certain tests: No
Comments on deviation from spec (you will still lose points, but it's better to let the grader know): No
General comments on design decisions: No

