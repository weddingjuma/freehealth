-- /***************************************************************************
-- *   FreeMedicalForms                                                      *
-- *   (C) 2008-2010 by Eric MAEKER, MD                                      *
-- *   eric.maeker@free.fr                                                   *
-- *   All rights reserved.                                                  *
-- *                                                                         *
-- *   This program is a free and open source software.                      *
-- *   It is released under the terms of the new BSD License.                *
-- *                                                                         *
-- *   Redistribution and use in source and binary forms, with or without    *
-- *   modification, are permitted provided that the following conditions    *
-- *   are met:                                                              *
-- *   - Redistributions of source code must retain the above copyright      *
-- *   notice, this list of conditions and the following disclaimer.         *
-- *   - Redistributions in binary form must reproduce the above copyright   *
-- *   notice, this list of conditions and the following disclaimer in the   *
-- *   documentation and/or other materials provided with the distribution.  *
-- *   - Neither the name of the FreeMedForms' organization nor the names of *
-- *   its contributors may be used to endorse or promote products derived   *
-- *   from this software without specific prior written permission.         *
-- *                                                                         *
-- *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   *
-- *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     *
-- *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS     *
-- *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE        *
-- *   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,  *
-- *   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,  *
-- *   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;      *
-- *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER      *
-- *   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT    *
-- *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN     *
-- *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
-- *   POSSIBILITY OF SUCH DAMAGE.                                           *
-- ***************************************************************************/
-- /**************************************************************************
-- *   Main Developper : Jim Busser, MD <jbusser@interchange.ubc.ca>         *
-- *   Contributors :                                                        *
-- *       Eric MAEKER, MD <eric.maeker@free.fr>                             *
-- ***************************************************************************/

-- /**
--  * \file canadian_db_finalize.sql
--  * \author Jim Busser, MD <jbusser@interchange.ubc.ca>, Eric MAEKER, MD <eric.maeker@free.fr>
--  * \version 0.4.4
--  * \date 14 July 2010
--  */

-- /**
--  * \brief This script creates the FreeDiams Canadian's drugs database itself.
--  *
--  * \warning SQL commands MUST end with \e ;
--  *
--  * Sept 22, 2010 : recreate the drug's UID using forms and routes ID.
--  *   Preparation of the future surrogate key.
--  *   Drugs are created by the FreeToolBox code.
--  *
--  * July 14, 2010 initial release
--  *
--  * NOTE: INSERT INTO "INFORMATIONS" (needs manual updating as to
--  *  i)  the version of the drug db that it is to be known as
--  *  ii) the compatible version of FreeDiams
--  *  iii) manual insertion into LK_MOL_ATC until scriptable
--  */


-- ********************************
-- Remove Veterinary drugs
-- ********************************
-- do the following for each of tables
-- form
-- route
-- ingred
-- ther
-- package

DELETE FROM drug
WHERE Class = "Veterinary"
;


-- ********************************
-- Feed FreeDiams table DRUGS
-- ********************************
-- these are Branded products
-- the drugs must be distinct on {drug or combination} plus strength)
-- note FreeDiam's DRUGS table needs its records pre-ordered ascending on column NAME

INSERT INTO DRUGS ("UID", "NAME")
SELECT DISTINCT
   A1.DRUG_CODE,
   A1.BRAND_NAME
FROM drug A1
ORDER BY A1.BRAND_NAME;


-- account for products that package >1 form and/or can be used by multiple routes
UPDATE DRUGS
SET FORM=
(SELECT group_concat(PHARMACEUTICAL_FORM, ", ")
FROM form A1
WHERE A1.DRUG_CODE=DRUGS.UID
GROUP BY DRUG_CODE
LIMIT 10);

UPDATE DRUGS
SET ROUTE=
(SELECT group_concat(ROUTE_OF_ADMINISTRATION, ", ")
FROM route A1
WHERE A1.DRUG_CODE=DRUGS.UID
GROUP BY DRUG_CODE
LIMIT 10);

-- ensure that any Canadian drug NULL strengths are backfilled from related ingredients

UPDATE DRUGS
SET GLOBAL_STRENGTH=
(SELECT group_concat(STRENGTH || STRENGTH_UNIT, ";")
FROM ingred A1
WHERE A1.DRUG_CODE=DRUGS.UID
GROUP BY DRUG_CODE
LIMIT 10);

-- set the ATC
-- note Canada's "ther" table contains nondistinct ATCs (to support multiple AHFS values)

UPDATE DRUGS
SET ATC=
(SELECT DISTINCT TC_ATC_NUMBER
FROM ther A1
WHERE A1.DRUG_CODE=DRUGS.UID
);


-- ********************************
-- Feed FreeDiams table COMPOSITION (molecular ingredients)
-- ********************************
-- note: some have DOSAGE_VALUE, others do not
-- also omitting in this step "MOLECULE_FORM"

INSERT INTO "COMPOSITION"
   ("UID", "MOLECULE_CODE", "MOLECULE_NAME", "DOSAGE", "DOSAGE_REF", "NATURE", "LK_NATURE")
SELECT DISTINCT
   A1.DRUG_CODE,
   A1.ACTIVE_INGREDIENT_CODE,
   A1.INGREDIENT,
   A1.STRENGTH || A1.STRENGTH_UNIT || "/" || A1.DOSAGE_VALUE || A1.DOSAGE_UNIT,
   "",
   "SA",
   1
FROM ingred A1
WHERE
   (A1.DOSAGE_VALUE != "")
   ;


INSERT INTO COMPOSITION
   ("UID", "MOLECULE_CODE", "MOLECULE_NAME", "DOSAGE", "DOSAGE_REF", "NATURE", "LK_NATURE")
SELECT DISTINCT
   A1.DRUG_CODE,
   A1.ACTIVE_INGREDIENT_CODE,
   A1.INGREDIENT,
   A1.STRENGTH || A1.STRENGTH_UNIT,
   "",
   "SA",
   1
FROM ingred A1
WHERE
   (A1.DOSAGE_VALUE = "");


-- Swith DRUG_CODE to DIN in DRUGS and COMPOSITION tables
UPDATE DRUGS SET UID =
(
  SELECT DIN FROM drug WHERE drug.DRUG_CODE=DRUGS.UID
);

UPDATE COMPOSITION SET UID =
(
  SELECT DIN FROM drug WHERE drug.DRUG_CODE=COMPOSITION.UID
);


-- ********************************
-- Feed INFORMATIONS (info about the drug data source)
-- ********************************
INSERT INTO "INFORMATIONS" (
    "VERSION",
    "NAME",
    "IDENTIFIANT" ,
    "COMPAT_VERSION",
    "PROVIDER",
    "WEBLINK",
    "COMPLEMENTARY_WEBSITE",
    "AUTHOR",
    "LICENSE",
    "LICENSE_TERMS",
    "DATE",
    "DRUG_UID_NAME",
    "PACK_MAIN_CODE_NAME",
    "ATC",
    "INTERACTIONS",
    "AUTHOR_COMMENTS",
    "LANGUAGE_COUNTRY",
    "DRUGS_NAME_CONSTRUCTOR")
VALUES (
    "0.5.2",
    "en=Canadian Drug Product Database
    fr=Base de données thérapeutique Canadienne
    xx=Canadian Drug Product Database
    ",
    "CA_HCDPD",
    "0.5.0",
    "HC: Health Canada Drug Product Database",
    "http://www.hc-sc.gc.ca/dhp-mps/prodpharma/databasdon/index-eng.php",
    "http://code.google.com/p/freemedforms/wiki/Database_ca",
    "Jim Busser, MD (CA) & Eric Maeker, MD (FR)",
    "(c) Her Majesty the Queen in Right of Canada",
    "Apply: http://publications.gc.ca/helpAndInfo/cc-dac/application-e.html on a no-charge, royalty-free, non-exclusive, annual renewable basis.",
    CURRENT_DATE,
    "DIN",
    "UPC",
    1,
    1,
    "While the download is technically open, the data is copyright. Please report any problem to the author.",
    "en_CA",
    "NAME (FORM, GLOBAL_STRENGTH)"
);


-- ********************************
-- Feed PACKAGING (available shipping sizes / quantities)
-- ********************************
-- Takes account of variability in
-- empty/non-empty fields within the source data

-- INSERT INTO PACKAGING (UID, PACKAGE_UID, LABEL, MARKETING)
-- SELECT A7.DIN, A7.UPC, A7.PRODUCT_INFORMATION, " "
-- FROM package A7
-- WHERE PACKAGE_SIZE = "" AND PRODUCT_INFORMATION != "";

-- INSERT INTO PACKAGING (UID, PACKAGE_UID, LABEL, MARKETING)
-- SELECT A7.DIN, A7.UPC, A7.PACKAGE_SIZE || " " || A7.PACKAGE_SIZE_UNIT || " " || A7.PACKAGE_TYPE, " "
-- FROM package A7
-- WHERE PACKAGE_SIZE != "" AND PRODUCT_INFORMATION = "";

-- INSERT INTO PACKAGING (UID, PACKAGE_UID, LABEL, MARKETING)
-- SELECT A7.DIN, A7.UPC, A7.PACKAGE_SIZE || " " || A7.PACKAGE_SIZE_UNIT || " " || A7.PACKAGE_TYPE || ", " || PRODUCT_INFORMATION, " "
-- FROM package A7
-- WHERE PACKAGE_SIZE != "" AND PRODUCT_INFORMATION != "";

-- INSERT INTO PACKAGING (UID, PACKAGE_UID, LABEL, MARKETING)
-- SELECT A7.DIN, A7.UPC, A7.PACKAGE_SIZE || " " || A7.PACKAGE_SIZE_UNIT || " " || A7.PACKAGE_TYPE || ", " || PRODUCT_INFORMATION, " "
-- FROM package A7
-- WHERE PACKAGE_SIZE = "" AND PRODUCT_INFORMATION = "";

-- the Canadian products mostly lack any unique UPC
-- a pseudo PACKAGE_UID is possible, but might be inconstant
-- UPDATE package SET UPC = ROWID;

-- ********************************
-- Drop the DPD staging tables
-- ********************************
DROP TABLE drug;
DROP TABLE form;
DROP TABLE route;
DROP TABLE status;
DROP TABLE ingred;
DROP TABLE ther;
DROP TABLE package;

--DROP TABLE form-abbr;

VACUUM;
