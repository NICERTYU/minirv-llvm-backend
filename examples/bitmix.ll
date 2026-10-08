define i32 @bitmix(i32 %a, i32 %b) {
  %both = and i32 %a, %b
  %either = or i32 %a, %b
  %mixed = xor i32 %both, %either
  ret i32 %mixed
}
