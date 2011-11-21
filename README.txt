NOTES:
The glsl stuff is commented out at the moment and not working at the moment...

BUILDING:
Edit the "rules" files in the RULES/ folder to suit your local environment and then run "make".

At Anima we have been building this against a version of Alembic based on
the 1.0.1 release.

USAGE:
// create node and connect time to it
string $aah = `createNode animaAlembicHolder`;
connectAttr time1.outTime ($aah+".time");

