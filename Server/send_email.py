import os
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail

message = open('./templates/december.html', 'r').read()

# Create the mail
mail = Mail(
    from_email='me@housefish.dev',
    to_emails='s10256965@connect.np.edu.sg',
    subject='Your Carbon Emissions - December',
    html_content=message
)

try:
    sg = SendGridAPIClient(os.environ.get('SENDGRID_API_KEY'))
    response = sg.send(mail)
    print(response.status_code)
    print(response.body)
    print(response.headers)
except Exception as e:
    print(e.message)
