-- passes its own checks but ends outside the keep, while it names the great hall as its final cell
return {
  name = "ends in the wrong cell",
  final_cell = "keep.hall",
  run = function(t)
    t.place(1, 0, 40)
    t.ticks(2)
  end,
}
