<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE xsl:stylesheet
[
   <!-- Namespace for XHTML -->
   <!ENTITY xhtmlns "http://www.w3.org/1999/xhtml">
]>

<!--
    Author      : Uwe Drechsel  <vym@InSilmaril.de>
    Description : transforms KDE Bookmarks into vym map
-->

<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:date="http://exslt.org/dates-and-times"
    extension-element-prefixes="date"
    xmlns="&xhtmlns;">


<xsl:output method="xml"
    doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
    doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"
    encoding="UTF-8"
    indent="yes"
    media-type="application/xhtml+xml"/>



<!-- ======================================= -->
<!-- 1 = true, 0 = false -->

<!-- Debuggin on/off? -->
<xsl:param name="debug" select="0"/>


<!-- ======================================= -->
<xsl:variable name="head.title">
   <xsl:choose>
      <xsl:when test="/vymmap/mapcenter/heading">
			
				<xsl:variable name="title">
				</xsl:variable>

        <xsl:value-of select="$title"/>
      </xsl:when>
      <xsl:otherwise></xsl:otherwise>
   </xsl:choose>
</xsl:variable>




<!-- ======================================= -->
<xsl:template match="*">
   <xsl:message>
      <xsl:text>WARNING: Unknown tag "</xsl:text>
      <xsl:value-of select="local-name(.)"/>
      <xsl:text>": </xsl:text>
      <xsl:value-of select="normalize-space(.)"/>
      <xsl:text>&#10;</xsl:text>
   </xsl:message>
</xsl:template>




<xsl:template match="xbel">
   <vymmap version="1.7.15" author="VYM - styles/kdebookmarks2vym.xsl" comment="" date="2006-04-24" backgroundColor="#ffffff" linkStyle="StylePolyLine" linkColor="#0000ff" defXLinkColor="#e6e6e6" defXLinkWidth="1">
	<mapcenter>
		<heading>Bookmarks</heading>
		<branch frameType="Rectangle">
			<heading>KDE</heading>
				  <xsl:apply-templates/>
		</branch>
	</mapcenter>	
   </vymmap>
</xsl:template>


<xsl:template match="folder">
   <branch scrolled="yes">
      <xsl:apply-templates/>
   </branch>
</xsl:template>

<xsl:template match="title">
   <heading>
      <xsl:apply-templates/>
   </heading>
</xsl:template>

<xsl:template match="separator">
   <branch>
		<heading>***************</heading>
   </branch>
</xsl:template>

<xsl:template match="desc">
   <htmlnote fonthint="var">
	<html>
		<body style="font-size:10pt;font-family:Sans Serif">
			<p>
				<xsl:value-of select="." />
			</p>
		</body>
	</html>

   </htmlnote>
</xsl:template>

<xsl:template match="bookmark">
   <branch>
		<xsl:attribute name="url" ><xsl:value-of select="@href" />
		</xsl:attribute>
		<xsl:apply-templates/>
   </branch>
</xsl:template>


<!-- Do nothing! We don't need some informational elements -->
<xsl:template match="info*"/>


</xsl:stylesheet>
