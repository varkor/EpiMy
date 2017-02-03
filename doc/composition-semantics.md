# EpiMy Compositional Semantics
An EpiMy program consists of a series of language blocks, denoted by `@[<Language>] { ... @}` tags within which is placed arbitrary, standalone, `<Language>` code, where `<Language>` is one of `Epilog` or `MysoreScript`.
If the code in a block would be error-free if executed alone in a standard compiler, it will have the same effect as part of an EpiMy program.

## Converting Datatypes between Languages
In various situations it is necessary to share data between the two languages. However, as data has different semantics, data must be converted between representation in the two languages.
### Objects and Compound Terms

## Epilog Semantics
### Functor Resolution
The built-in `call/3(+Name, +Arguments, ?Return)` takes the name of a function and a list of arguments and calls the corresponding MysoreScript method, should it exist (otherwise an error is thrown as if an inexistent MysoreScript function was called natively). If `Return` is bound, call resolves to `true` if the converted MysoreScript return type unifies with `Return`, otherwise `Return` is bound as expected.

### Variable Resolution
The semantics of variables in Epilog are not affected, so that they operate as expected when binding. However, variables from an outer scope in a MysoreScript can be accessed through the built-in `var/2(+Name, ?Value)` provided by EpiMy in addition to the standard library. Given an atom, `Name`, `Value` will bind to the value of the variable with the given name in the enclosing MysoreScript scope. (Note that to bind to variables starting with a capital letter in MysoreScript, a quoted atom must be used.)

## MysoreScript Semantics
### Function Resolution
The global function `unify(name: String, parameters: Array) -> [bindings: Array]?` takes the name of an Epilog clause and an array of parameters, and  and attempts unification of `<name>(<...parameters>).`, where `null` parameters get mapped to new variables.
If unification does not succeed, the function call returns nothing (which can be checked with a truth check), otherwise the returned array contains the final values of the argument registers corresponding to the unification.
