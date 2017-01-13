# EpiMy Compositional Semantics
An EpiMy program consists of a series of language blocks, denoted by `@[<Language>] { ... @}` tags within which is placed arbitrary, standalone, `<Language>` code, where `<Language>` is one of `Epilog` or `MysoreScript`.
If the code in a block would be error-free if executed alone in a standard compiler, it will have the same effect as part of an EpiMy program.

## Converting Datatypes between Languages
In various situations it is necessary to share data between the two languages. However, as data has different semantics, data must be converted between representation in the two languages.
### Objects and Compound Terms

## Epilog Semantics
### Functor Resolution
In a rule or query body, goal unification occurs as usual. However, upon encountering a goal term such that no rule with a matching functor has be defined, the compiler will look for a matching MysoreScript function in the scope of the current Epilog block. This is a function such that:
- It has the same name as the goal functor.
- It either:
    - has the same number of arguments as the goal term.
    - has one fewer arguments than the goal term.

If such a function is found, it will unify with the goal, if and only if:
- It has the same number of arguments as the goal term and, when passed each of the goal's parameters as arguments, returns a value that is logically equivalent to `true`.
- It has one fewer arguments than the goal term and, when passed each bar the final of the goal's parameters as arguments, returns a value that unifies with the ultimate parameter of the goal term.

## MysoreScript Semantics
### Function Resolution
Upon reaching an function application in MysoreScript, if the function name is in scope, it will be executed as usual. However, if it cannot be found, a matching Epilog clause will be sought. This is a clause such that:
- Its functor name is the same as the function name.
- It either:
    - has the same number of arguments as the number of goal parameters.
    - has one fewer arguments than the number of goal parameters.

If such a clause is found, the application will evaluate to the value resulting from applying the following rule:
- If it has the same number of arguments as goal parameters, it will return `true` if the goal unifies when passed each of the arguments as parameters, or `false` if it does not.
- If it has one fewer arguments than the number of goal paremeters, it will return the value of the bound final parameter of the goal when passed each of the arguments as the first parameters, or `null` if the parameter is unbound.
