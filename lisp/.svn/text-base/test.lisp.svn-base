'QUOTE
(quote a)
(quote (a b c))
(quote (a (b c)))
(quote)



'ATOM
(atom a)
(atom 'a)
(atom ())
(atom '())
(atom (a b c))
(atom '(a b c))
(atom)



'EQ
(eq 'a 'a)
(eq 'a 'b)
(eq '() '())
(eq '(a b) 'a)
(eq 'a '(b c))
(eq)



'CAR
(car '(a b c))
(car '())
(car 'a)
(car)



'CDR
(cdr '(a b c))
(cdr '(a b))
(cdr '(a))
(cdr '())
(cdr 'a)
(cdr)



'CONS
(cons 'a '(b c))
(cons 'a '(b))
(cons 'a '())
(cons 'a 'b)
(cons '(a) '(b c))
(cons '(a) '(b))
(cons '(a) '())
(cons '(a) 'b)
(cons '(a x) '(b c))
(cons '(a x) '(b))
(cons '(a x) '())
(cons '(a x) 'b)
(cons)




'COND
(cond ((atom 'a) (cdr '(a b c))) ('t 'second))
(cond ('() 'first))
(cond)




'LAMBDA
((lambda (x) (cons x '(b))) 'a)

((lambda (x y) (cons x (cdr y))) 'z '(a b c))

 ((lambda (f) (f '(b c)))
  '(lambda (x) (cons 'a x)))


'LABEL
(label subst (lambda (x y z)
               (cond ((atom z)
                      (cond ((eq z y) x)
                            ('t z)))
                     ('t (cons (subst x y (car z))
                               (subst x y (cdr z)))))))






'DEFUN
(defun subst. (x y z)
  (cond ((atom z)
         (cond ((eq z y) x)
               ('t z)))
        ('t (cons (subst. x y (car z))
                  (subst. x y (cdr z)))))))
(subst. 'm 'b '(a b (a b c) d))

(defun pair. (x y)
  (cond ((and (null x) (null y)) '())
        ((and (not (atom x)) (not (atom y)))
         (cons (list (car x) (car y))
               (pair. (cdr x) (cdr y))))))
(pair. '(x y z) '(a b c))





'EXEC
(exec '(ls))
(exec '(ls) '(/etc))
(exec 'ls '/xxxx)


'UTF8
(cdr '(Привет мир !))

