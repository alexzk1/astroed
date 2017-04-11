#!/usr/bin/perl
use File::Find;


print<<EOL;
<!DOCTYPE RCC><RCC version="1.0">
  <qresource prefix="/">
EOL

sub wanted {
   
   if ( !(-d))
   {
      print "\t<file>";
      print $File::Find::name;
      print "</file>\n";
   }
}

find(\&wanted, './luarc/');

print<<EOL;
  </qresource>
</RCC>
EOL

