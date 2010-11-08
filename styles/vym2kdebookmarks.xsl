<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE xsl:stylesheet
[
   <!-- Namespace for XHTML -->
   <!ENTITY xhtmlns "http://www.w3.org/1999/xhtml">
]>

<!--
    Author      : Uwe Drechsel  <vym@InSilmaril.de>
	Credits to  : Thomas Schraitle for his patience in explaining XSL to me
    Description : transforms vym maps into KDE Bookmarks
-->

<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:date="http://exslt.org/dates-and-times"
    extension-element-prefixes="date">


<xsl:output method="xml"
    doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
    doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"
    encoding="UTF-8"
    indent="yes"
    media-type="application/xhtml+xml"/>

<!-- ======================================= -->

<xsl:template match="/">
  <xsl:apply-templates/>
</xsl:template>


<xsl:template match="text()"/>


<xsl:template match="vymmap">
  <xbel>
    <xsl:apply-templates/>
  </xbel>
</xsl:template>


<xsl:template match="mapcenter">

  <xsl:apply-templates/>
</xsl:template>


<xsl:template match="branch">
   <xsl:choose>
     <xsl:when test="heading='KDE'">
        <xsl:apply-templates select="branch" mode="kde"/>
     </xsl:when>
     <xsl:when test=". = 'Firefox'">
        <xsl:apply-templates mode="firefox"/>
     </xsl:when>
     <!-- ... -->
     <xsl:otherwise>
       <xsl:apply-templates/>
     </xsl:otherwise>
   </xsl:choose>

  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="heading" mode="kde">
      <title>		
      <xsl:value-of select="normalize-space (.)"/>
	  </title>

   <xsl:choose>
     <xsl:when test=". = 'KDE'">
        <xsl:apply-templates mode="kde"/>
     </xsl:when>
     <xsl:otherwise>
       <xsl:apply-templates/>
     </xsl:otherwise>
   </xsl:choose>

  <xsl:apply-templates/>
</xsl:template>


<xsl:template match="branch" mode="kde">
	<xsl:choose>
		<xsl:when test="@url">
			<xsl:element name="bookmark">	
				<xsl:attribute name="href" ><xsl:value-of select="@url" />
				</xsl:attribute>
				<xsl:apply-templates mode="kde"/>
			</xsl:element>
		</xsl:when>
		<xsl:otherwise>
			<xsl:choose>
				<xsl:when test="contains(heading,'***')">
					<separator folded="yes" />
				</xsl:when>
				<xsl:otherwise>
					<folder folded="yes" icon="bookmark_folder">
						<xsl:apply-templates mode="kde"/>
					</folder>
				</xsl:otherwise>
			</xsl:choose>	

	</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<!--
		  <xsl:text>&#10;</xsl:text>

				<xsl:message> WARNING: No @url attribute given of "<xsl:value-of select="normalize-space(heading)"/>"</xsl:message>

<xsl:template match="branch">
  <xsl:choose>
    <xsl:when test="@url">
      <xsl:text> *nokde* </xsl:text>
      <xsl:value-of select="@url"/>
      <xsl:text>&#10;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message> WARNING: No @url attribute given of "<xsl:value-of select="normalize-space(heading)"/>"</xsl:message>
    </xsl:otherwise>
  </xsl:choose>

  <xsl:apply-templates/>

</xsl:template>
-->


</xsl:stylesheet>
