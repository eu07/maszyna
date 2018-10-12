unit klasy_ham;          {fizyka hamulcow dla symulatora}

(*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*)
(*
    MaSzyna EU07 - SPKS
    Brakes. Basic classes.
    Copyright (C) 2007-2013 Maciej Cierniak
*)


interface

uses mctools,sysutils;

//CONST;

TYPE
    TRurka= class  //bezposrednie polaczenie, co wleci to wyleci
      private
        Vol: real;
        Cap: real;

      public
        function Update(dt: real);
        procedure Flow(dV: real);
      end;

    PRurka= ^TRurka;

    TTrojnik= class(TRurka)
      private
        Wyjscie1: PRurka;
        Wyjscie2: PRurka;
        Wyjscie3: PRurka;
      public
      end;

    TZbiornik= class(TRurka)
      private
        Wyjscie_jeden: PRurka;
      public
      end;

    TPowtarzacz= class(TRurka)
      private
      public
      end;

    TPrzekladnik= class(TPowtarzacz)
      private
      public
      end;

function PF(P1,P2,S:real):real;

implementation



end.


