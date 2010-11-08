<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet name="VYM_TaskJuggler" version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text"/>
<xsl:template match="node()">
	<xsl:apply-templates/>
</xsl:template>
<xsl:template match="/vymmap/mapcenter">
project <xsl:value-of select="translate(heading, ' ', '_')"/> "<xsl:value-of select="heading"/>" "1.0" 2002-01-16 2002-05-28 {
  # Pick a day during the project that will be reported as 'today' in
  # the project reports. If not specified the current day will be
  # used, but this will likely be ouside of the project range, so it
  # can't be seen in the reports.
  now 2002-03-05-13:00
  # Hide the clock time. Only show the date.
  timeformat "%Y-%m-%d"
  # The currency for all money values is U.S. Dollars.
  currency "USD"

  # We want to compare the planned scenario, to one with the actual
  # scenario
  scenario plan "Planned" {
    scenario actual "Actual"
  }
}
# The daily default rate of all resources. This can be overriden for each
# resource. We specify this, so that we can do a good calculation of
# the costs of the project.
rate 310.0

# This is one way to form teams
macro allocate_developers [
  allocate dev1
  allocate dev2 { limits { dailymax 4h } }
  allocate dev3
]

flags team

resource dev "In House" {
  resource dev1 "Some Guy" { rate 330.00 }
  resource dev2 "Some Other Guy"
  resource dev3 "Some Last Guy on Vacation" { vacation 2002-02-01 - 2002-02-05 }

  flags team
}

resource misc "Outsource" {
  resource test "Out Sourcer1" { limits { dailymax 6.4h } rate 240.00 }
  resource doc  "Out Source2" { rate 280.00 vacation 2002-03-11 - 2002-03-16 }

  flags team
}

# In order to do a simple profit and loss analysis of the project we
# specify accounts. One for the development costs, one for the
# documentation costs and one account to credit the customer payments
# to.
account dev "Development" cost
account doc "Documentation" cost
account rev "Payments" revenue

# Now we specify the work packages. The whole project is described as
# a task that contains sub tasks. These sub tasks are then broken down
# into smaller tasks and so on. The innermost tasks describe the real
# work and have resources allocated to them. Many attributes of tasks
# are inherited from the enclosing task. This saves you a lot of
# writing.
task  <xsl:value-of select="translate(heading, ' ', '_')"/> "<xsl:value-of select="heading"/>" {

  # All work related costs will be booked to this account unless the
  # sub tasks specifies it differently.
  account dev

	<xsl:call-template name="recursive"/>

}

# This task report is for use with the TaskJuggler GUI
taskreport "Project Overview" {
  columns start, end, effort, duration, completed, status, note, cost, revenue
  scenario actual
}

# A resource report for use with the TaskJuggler GUI
resourcereport "Resource Usage" {
  columns effort, freeload, utilization, rate
  scenario actual
  hideresource 0
}

# For conveniance we would like each report to contain links to the
# other reports. So we declare a macro with a fragment of raw HTML
# code to be embedded into all the HTML reports.
macro navbar [
rawhead
  '<table align="center" border="2" cellpadding="10"
    style="background-color:#f3ebae; font-size:105%">
  <tr>
    <td><a href="Tasks-Overview.html">Tasks Overview</a></td>
    <td><a href="Staff-Overview.html">Staff Overview</a></td>
    <td><a href="Accounting.html">Accounting</a></td>
    <td><a href="Calendar.html">Calendar</a></td>
  </tr>
  <tr>
    <td><a href="Tasks-Details.html">Tasks Details</a></td>
    <td><a href="Staff-Details.html">Staff Details</a></td>
    <td><a href="Status-Report.html">Status Report</a></td>
    <td><a href="acso.eps">GANTT Chart (Postscript)</a></td>
  </tr>
  </table>
  <br/>'
]

# As the first report, we would like to have a general overview of all
# tasks with their computed start and end dates. For better
# readability we include a calendar like column that lists the effort
# for each week.
htmltaskreport "Tasks-Overview.html" {
  # This report should contain the navigation bar we have declared
  # above.
  ${navbar}
  # The report should be a table that contains several columns. The
  # task and their information form the rows of the table. Since we
  # don't like the title of the effort column, we change it to "Work".
  columns hierarchindex, name, duration, effort { title "Work"},
          start, end, weekly
  # For this report we like to have the abbreviated weekday in front
  # of the date. %a is the tag for this.
  timeformat "%a %Y-%m-%d"

  # Don't show load values.
  barlabels empty
  # Set a title for the report
  headline "<xsl:value-of select="heading"/> Project"
  # And a short description what this report is about.
  caption "This table presents a management-level overview of the project. The values are days or man-days."
}

# Now a more detailed report that shows all jobs and the people
# assigned to the tasks. It also features a comparison of the planned
# and actual scenario.
htmltaskreport "Tasks-Details.html" {
  ${navbar}
  # Now we use a daily calendar.
  columns no, name, start, end, scenario, daily
  #start 2002-03-01
  #end 2002-04-01
  # Show plan and delayed scenario values.
  scenarios plan, actual
  headline "<xsl:value-of select="heading"/> Project - March 2002"
  caption "This table shows the load of each day for all the tasks.
  Additionally the resources used for each task are listed. Since the
  project start was delayed, the delayed schedule differs significantly
  from the original plan."
  # Don't hide any resources, that is show them all.
  hideresource 0
}

# The previous report listed the resources per task. Now we generate a
# report the lists all resources.
htmlresourcereport "Staff-Overview.html" {
  ${navbar}
  # Add a column with the total effort per task.
  columns no, name { cellurl "http://www.tj.org" }, scenario, weekly, effort
  scenarios plan, actual
  # Since we want to see the load values as hours per week, we switch
  # the unit that loads are reported in to hours.
  loadunit hours
  headline "Weekly working hours for the <xsl:value-of select="heading"/> Project"
}

# Now a report similar to the above one but with much more details.
htmlresourcereport "Staff-Details.html" {
  ${navbar}
  columns name, daily, effort
  # To still keep the report readable we limit it to show only the
  # data for March 2002.
  start 2002-01-16
  end 2002-04-01
  hidetask 0
  # The teams are virtual resources that we don't want to see. Since
  # we have assigned a flag to those virtual resource, we can just
  # hide them.
  hideresource team
  # We also like to have the report sorted alphabetically ascending by
  # resource name.
  sortresources nameup
  loadunit hours
  headline "Daily working hours for the <xsl:value-of select="heading"/> Project - March 2002"
}

htmlweeklycalendar "Calendar.html" {
  ${navbar}
  headline "Ongoing Tasks - March 2002"
  start 2002-03-01
  end 2002-04-01
}

htmlstatusreport "Status-Report.html" {
  ${navbar}
}

# To conclude the HTML reports a report that shows how badly the
# project is calculated is generated. The company won't get rich with
# this project. Due to the slip, it actually needs some money from the
# bank to pay the salaries.
htmlaccountreport "Accounting.html" {
  ${navbar}
  # Besides the number of the account and the name we have a column
  # with the total values (at the end of the project) and the values
  # for each month of the project.
  columns no, name, scenario, total, monthly
  headline "P&#038;L for the Accounting Software Project"
  caption "The table shows the profit and loss
           analysis as well as the cashflow situation of the Accounting
           Software Project."
  # Since this is a cashflow calculation we show accumulated values
  # per account.
  accumulate
  scenarios plan, actual
}

# Finally we generate an XML report that contains all info about the
# scheduled project. This will be used by tjx2gantt to create a nice
# Gantt chart of our project.
xmlreport "<xsl:value-of select="translate(heading, ' ', '_')"/>.tjx" {
# version 2
}
</xsl:template>
<xsl:template name="recursive">
	<xsl:for-each select="branch">
        	task <xsl:value-of select="translate(heading, ' ', '_')"/> "<xsl:value-of select="heading"/>" {
		# I've included all of the Optional Attributes here.
		# Commented out for your pleasure.	
		#account
		#allocate dev1
		#complete
		#depends 
		#duration
		#effort 20d
		#endbuffer
		#endcredit
		#end, flags
		#journalentry
		#length
		#maxend
		#maxstart
		#milestone
		#minend
		#minstart
		#note
		#precedes
		#priority
		#projectid
		#reference
		#responsible
		#scheduled
		#scheduling
		#shift
		#startbuffer
		#startcredit
		plan:start 2002-03-05
		actual:start 2002-03-05
		#statusnote
		#supplement
		<xsl:call-template name="recursive"/>
		}
	</xsl:for-each>
</xsl:template>
</xsl:stylesheet>