# Security Policy

The Xenomai community takes the security of its code seriously. If you think
you have found a security vulnerability, please read the next sections and
follow the instructions to report your finding.

## Security Context

Open source software can be used in various contexts that may go far beyond
what it was originally designed and also secured for. Therefore, we describe
here how Xenomai is currently expected to be used in security-sensitive
scenarios.

Access to the real-time core of Xenomai is considered a privileged operation.
Therefore, typical real-time applications are running as root user or with
otherwise raised privileges as they can, by nature, dominate CPU resources via
the real-time core. Xenomai does not provide any secure temporal partitioning
mechanisms but rather expects all real-time applications to collaboratively
use its services. While the core foresees granting non-root users access to
the real-time core as well, there are no guarantees provided to isolate
different users from each other regarding theses resources.

Still, the Xenomai real-time core is designed and implemented to maintain
isolation mechanisms regarding regular Linux resources. Any violation of this
would be considered by us as a vulnerability.

## Reporting a Vulnerability

Please DO NOT report any potential security vulnerability via a public channel
(mailing list, public gitlab issue etc.). Instead, create a report via
https://gitlab.com/Xenomai/xenomai3/xenomai/-/work_items/new?issue%5Bissue_type%5D=incident&issue%5Bconfidential%5D=true
or contact the maintainers jan.kiszka@siemens.com and rpm@xenomai.org via email
directly. Please provide a detailed description of the issue, the steps to
reproduce it, the affected versions and, if already available, a proposal for a
fix. You should receive a response within 10 working days. If the issue is
confirmed as a vulnerability by us, we will request a CVE via GitLab, publish
the fix and give credits for your report if desired. This project follows a 90
day disclosure timeline.
