; (x + 7) * 4 - y, with the multiply expressed as a shift.
define i32 @arithmetic(i32 %x, i32 %y) {
entry:
  %sum = add i32 %x, 7
  %scaled = shl i32 %sum, 2
  %answer = sub i32 %scaled, %y
  ret i32 %answer
}
