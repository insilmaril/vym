print("Current Heading: " + model.getHeadingPlainText() );
/*
if ( model.getHeadingPlainText() == "foobar" )
{
  print("  ...is 'foobar'");
}   else    
{
  print("  ...is NOT 'foobar'");
}

//vym.toggleTreeEditor()
*/
print ("Current map: " + model.getFileName() );

m1 = vym.getCurrentMap();
print ("m1: " + m1.getFileName() );
vym.selectMap(1);
m2 = vym.getCurrentMap();
print ("m1: " + m1.getFileName() );
print ("m2: " + m2.getFileName() );
vym.selectMap(0);
m3 = vym.getCurrentMap();
print ("m1: " + m1.getFileName() );
print ("m2: " + m2.getFileName() );
print ("m3: " + m3.getFileName() );