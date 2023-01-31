# Unbalanced PSI from SimplePIR

## TODO

 - layout basic architectural plan
 - decide datatype for psi
   - does this need to play nice with SimplePIR?
 - update randomness generation
 - determine project structure
 - figure out cmake
 - figure out how to:
   - use general hash functions
   - use the hash function
   - call into SimplePIR
   - do the DDH exponentiation


## Open Questions
 - how to call into SimplePIR?
 - can we do an actual stateless server-client architecture rather than two-way communication channel?
   - how does SimplePIR interact with this?


## Notes to Self
Check for memory leaks: `valgrind --leak-check=yes ./main`


