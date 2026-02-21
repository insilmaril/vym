* Before 1.4.6
    - <note> is used. Data is read 
        - from external files mentioned in href attribute
          and inserted between <body> elements
        - from characters
      Not clear: Do empty characters always overwrite the previously set text from file?

* Before 2.5.0
    - No CDATA to save richText in Heading
    - setting only as attribut

* 2.5.0 to 2.7.562
    - HTML data encoded as CDATA (heading and vymnote)

* After 2.7.562
    - vymText is used (at least headgin)


* pre 1.13.2 <xlink> elements used to be within <branch> and did not
    have control points yet


<note>      -> <vymnote>
    - can have <html> 
<htmlnote>  -> <vymnote>
    - can have<html> 



