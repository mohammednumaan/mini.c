articles i referenced to build this project:

- https://www.youtube.com/watch?v=2htbIR2QpaM
- https://educatedguesswork.org/posts/memory-management-1/
- https://rahalkar.dev/posts/2025-10-25-writing-memory-allocator-heap-internals/
- https://www.thejat.in/learn/memory-alignment
- https://stackoverflow.com/questions/12825148/what-is-the-meaning-of-the-term-arena-in-relation-to-memory
- https://www.reddit.com/r/computerscience/comments/gst3bj/what_does_64_bit_cpu_actually_means/

questions i have:

- why can't we allocate dynamic-sized objects on the stack? what problems and solutions do we have to deal with when we want to do that?\
`source`: https://www.reddit.com/r/ProgrammingLanguages/comments/qilbxf/whats_so_bad_about_dynamic_stack_allocation/


implelmentation ideas:

- build a simplified version of the memory allocator by learning through the `glibc` malloc implementation/internals. (refer article)

- 3 core principles:
    1. chunk layout
    2. bin system
    3. arena model 
