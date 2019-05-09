if(ops[1] > ops[0]) {
back_array[1] = true;
back_array[0] = false;
return ops[0];
}

else {
back_array[1] = false;
back_array[0] = true;
return ops[1];
}


//return back_array[1]; //Vignesh - when you want this instruction to backpressurize on an edge, I think we should put backpressure on the unpropogated edge... but?
